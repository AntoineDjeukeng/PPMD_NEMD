#include "pc.h"
#include <stdio.h>

void fill_frame(t_frame *f, int step)
{
    /* Populate a simple, consistent snapshot for demo */
    f->head = NULL;
    f->tail = NULL;
    f->size = 0;
    f->cap = 4;
    f->kind = TRAJ_KIND_UNKNOWN;
    f->nframes = (long long)(step + 1);
    f->natoms = 8;
    f->ctx = NULL;
}

void process_frame(const t_frame *f)
{
    printf("Consumer %lu frames=%lld natoms=%d size=%d cap=%d\n",
           (unsigned long)pthread_self(),
           f->nframes, f->natoms, f->size, f->cap);
}

void process_frame_subset(const t_frame *f, const t_topo *topo, int start, int count)
{
    int end;
    long long fr;
    int tot;

    (void)f;
    fr = f ? f->nframes : 0;
    tot = topo ? topo_count_mols(topo) : 0;
    if (start < 0)
        start = 0;
    if (count < 0)
        count = 0;
    end = start + count - 1;
    if (count == 0)
        end = -1;
    printf("Consumer %lu frames=%lld mols[%d..%d]/%d\n",
           (unsigned long)pthread_self(), fr, start, end, tot);
}
