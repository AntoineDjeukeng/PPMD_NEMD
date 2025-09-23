#include "pc.h"
#include <string.h>

typedef struct s_slot 
{
    t_frame    frame;
    atomic_int refs;
    unsigned   gen;
} t_slot;

static t_slot          g_slots[SLOTS];
static int             g_pub_idx = -1;
static unsigned        g_pub_gen = 0;
static pthread_mutex_t g_pub_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_pub_cv  = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_reuse_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_reuse_cv  = PTHREAD_COND_INITIALIZER;

void ring_init(void)
{
    int i;

    for (i = 0; i < SLOTS; i++) {
        atomic_init(&g_slots[i].refs, 0);
        g_slots[i].gen = 0;
        memset(&g_slots[i].frame, 0, sizeof(g_slots[i].frame));
    }
}

void ring_wait_slot_free(int k)
{
    pthread_mutex_lock(&g_reuse_mtx);
    while (atomic_load_explicit(&g_slots[k].refs, memory_order_acquire) != 0)
        pthread_cond_wait(&g_reuse_cv, &g_reuse_mtx);
    pthread_mutex_unlock(&g_reuse_mtx);
}

void ring_publish(int k)
{
    atomic_store_explicit(&g_slots[k].refs, NUM_CONSUMERS, memory_order_release);
    pthread_mutex_lock(&g_pub_mtx);
    g_pub_idx = k;
    g_pub_gen += 1;
    g_slots[k].gen = g_pub_gen;
    pthread_cond_broadcast(&g_pub_cv);
    pthread_mutex_unlock(&g_pub_mtx);
}

void ring_wait_new_gen(int *k, unsigned *gen_out)
{
    static __thread unsigned seen_gen = 0;
    unsigned my_gen;

    pthread_mutex_lock(&g_pub_mtx);
    while (g_pub_gen == seen_gen)
        pthread_cond_wait(&g_pub_cv, &g_pub_mtx);
    if (k)
        *k = g_pub_idx;
    my_gen = g_pub_gen;
    seen_gen = my_gen;
    pthread_mutex_unlock(&g_pub_mtx);
    if (gen_out)
        *gen_out = my_gen;
}

const t_frame* ring_frame(int k)
{
    int idx;
    const t_frame *ret;
    int refs;

    idx = k;
    if (idx < 0)
        return NULL;
    if (idx >= SLOTS)
        return NULL;

    refs = atomic_load_explicit(&g_slots[idx].refs, memory_order_acquire);
    if (refs < 0)
        return NULL;

    ret = &g_slots[idx].frame;
    if (!ret)
        return NULL;
    return ret;
}

t_frame* ring_frame_mut(int k)
{
    int idx;
    t_frame *ret;
    int refs;

    idx = k;
    if (idx < 0)
        return NULL;
    if (idx >= SLOTS)
        return NULL;

    refs = atomic_load_explicit(&g_slots[idx].refs, memory_order_acquire);
    if (refs < 0)
        return NULL;

    ret = &g_slots[idx].frame;
    if (!ret)
        return NULL;
    return ret;
}

void ring_ack(int k)
{
    if (atomic_fetch_sub_explicit(&g_slots[k].refs, 1, memory_order_acq_rel) == 1) {
        pthread_mutex_lock(&g_reuse_mtx);
        pthread_cond_broadcast(&g_reuse_cv);
        pthread_mutex_unlock(&g_reuse_mtx);
    }
}

int ring_slot_for_gen(unsigned gen)
{
    int i;

    for (i = 0; i < SLOTS; i++) {
        if (g_slots[i].gen == gen)
            return i;
    }
    return -1;
}

unsigned ring_slot_gen(int k)
{
    unsigned g;

    if (k < 0)
        return 0;
    if (k >= SLOTS)
        return 0;
    g = g_slots[k].gen;
    return g;
}

int ring_slot_refs(int k)
{
    int r;

    if (k < 0)
        return -1;
    if (k >= SLOTS)
        return -1;
    r = atomic_load_explicit(&g_slots[k].refs, memory_order_acquire);
    return r;
}

/* two-slot mode: missed-ack helper not required */
