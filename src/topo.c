#include "pc.h"
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t g_topo_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_topo_cv  = PTHREAD_COND_INITIALIZER;
static int             g_topo_ready = 0;
static t_frame0        g_topo;

static void topo_init_defaults(t_frame0 *t)
{
    int i;

    t->natoms = 0;
    i = 0;
    while (i < 3) {
        t->box[i].x = 0.0;
        t->box[i].y = 0.0;
        t->box[i].z = 0.0;
        i++;
    }
    t->x = NULL;
    t->sum.v = 0.0;
}

void topo_build_from_traj(t_frame0 *dst, const t_frame *src)
{
    int n;
    int i;

    topo_init_defaults(dst);
    n = src ? src->natoms : 0;
    if (n < 0)
        n = 0;
    dst->natoms = n;
    dst->box[0].x = 1.0; dst->box[0].y = 0.0; dst->box[0].z = 0.0;
    dst->box[1].x = 0.0; dst->box[1].y = 1.0; dst->box[1].z = 0.0;
    dst->box[2].x = 0.0; dst->box[2].y = 0.0; dst->box[2].z = 1.0;
    dst->x = NULL;
    if (n > 0)
        dst->x = (t_vec3*)malloc((size_t)n * sizeof(t_vec3));
    if (dst->x) {
        i = 0;
        while (i < n) {
            dst->x[i].x = 0.0;
            dst->x[i].y = 0.0;
            dst->x[i].z = 0.0;
            i++;
        }
    }
    dst->sum.v = (double)n;
}

void topo_copy(t_frame0 *dst, const t_frame0 *src)
{
    int n;
    int i;

    topo_init_defaults(dst);
    if (!src)
        return;
    n = src->natoms;
    dst->natoms = n;
    dst->box[0] = src->box[0];
    dst->box[1] = src->box[1];
    dst->box[2] = src->box[2];
    dst->sum = src->sum;
    dst->x = NULL;
    if (n > 0)
        dst->x = (t_vec3*)malloc((size_t)n * sizeof(t_vec3));
    if (!dst->x)
        return;
    i = 0;
    while (i < n) {
        dst->x[i] = src->x ? src->x[i] : (t_vec3){0.0, 0.0, 0.0};
        i++;
    }
}

void topo_free(t_frame0 *t)
{
    if (!t)
        return;
    free(t->x);
    t->x = NULL;
}

void topo_publish_global(const t_frame0 *src)
{
    pthread_mutex_lock(&g_topo_mtx);
    topo_free(&g_topo);
    topo_copy(&g_topo, src);
    g_topo_ready = 1;
    pthread_cond_broadcast(&g_topo_cv);
    pthread_mutex_unlock(&g_topo_mtx);
}

void topo_wait_global_copy(t_frame0 *dst)
{
    pthread_mutex_lock(&g_topo_mtx);
    while (!g_topo_ready)
        pthread_cond_wait(&g_topo_cv, &g_topo_mtx);
    topo_copy(dst, &g_topo);
    pthread_mutex_unlock(&g_topo_mtx);
}

