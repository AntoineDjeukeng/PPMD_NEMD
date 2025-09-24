#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "traj.h"
#include "traj_batch.h"

void ft_print_ring_slots_raw(const traj_ring *R)
{
    if (!R || R->cap <= 0) return;
    printf("raw slots from start=%d (cap=%d, count=%d)\n", R->start, R->cap, R->count);
    for (int i = 0; i < R->cap; ++i) {
        int idx = (R->start + i) % R->cap;
        const t_frame *fr = &R->slot[idx];
        printf("slot[%3d]: step=%d time=%.4f\n", idx, fr->step, fr->time);
        if (fr->x && fr->natoms > 0) {
            for (int j = 0; j < 4; ++j) {
                printf("  atom[%4d]: %8.3f %8.3f %8.3f\n", fr->natoms-1-j, fr->x[fr->natoms-1-j][0], fr->x[fr->natoms-1-j][1], fr->x[fr->natoms-1-j][2]);
            }
        }
        if(R->kind==TRAJ_KIND_TRR)
        {
            if (fr->v && fr->natoms > 0) {
                for (int j = 0; j < 4; ++j) 
                {
                    printf("  vel [%4d]: %8.3f %8.3f %8.3f\n", fr->natoms-1-j, fr->v[fr->natoms-1-j][0], fr->v[fr->natoms-1-j][1], fr->v[fr->natoms-1-j][2]);
                }
            }
            if (fr->f && fr->natoms > 0) {
                for (int j = 0; j < 4; ++j) {
                    printf("  force[%4d]: %8.3f %8.3f %8.3f\n", fr->natoms-1-j, fr->f[fr->natoms-1-j][0], fr->f[fr->natoms-1-j][1], fr->f[fr->natoms-1-j][2]);
                }
            }
        }
    }
}


int main(int ac, char **av)
{
    setlocale(LC_ALL,"");
    if (ac < 2) {
        fprintf(stderr,
            "usage: %s [-C cap] [-K win] file.(xtc|trr)\n"
            "  -C cap  ring capacity to keep (default 3)\n"
            "  -K win  sliding window size to process (default 3, <= cap)\n", av[0]);
        return 1;
    }

    traj_ring *traject;
    traject = (traj_ring *)calloc(1, sizeof(traj_ring));
    if (!traject) {
        fprintf(stderr, "error: out of memory\n");
        return 1;
    }

    int cap = 6, K = 3;
    int argi = 1;
    while (argi+1 < ac && av[argi][0]=='-') {
        if (!strcmp(av[argi],"-C")) { cap = atoi(av[argi+1]); argi+=2; continue; }
        if (!strcmp(av[argi],"-K")) { K   = atoi(av[argi+1]); argi+=2; continue; }
        break;
    }
    if (argi >= ac) { fprintf(stderr,"error: missing input file\n"); return 1; }
    if (cap <= 0)   { fprintf(stderr,"error: cap must be > 0\n"); return 1; }
    if (K <= 0)     { fprintf(stderr,"error: K must be > 0\n"); return 1; }
    const char *path = av[argi];
    if (argi+1 < ac) fprintf(stderr,"note: ignoring %d extra arg(s); opening only %s\n", ac-argi-1, path);


    if(traj_ring_init(traject, cap, path)!=exdrOK)
    {
        traj_ring_free(traject);
        fprintf(stderr,"ring init failed\n");
        return 1;
    }
    int rc;
    rc=exdrOK;
    while (rc==exdrOK)
    {
        rc=traj_ring_read_next(traject);
        if(rc==exdrOK)
            ft_print_ring_slots_raw(traject);
    }
    traj_ring_free(traject);
    return 0;
}
