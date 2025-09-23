#include "pc.h"

static int next_k(int k)
{
    if (SLOTS == 2)
        return k ^ 1;
    return (k + 1) % SLOTS;
}

static void produce_once(int k, int step)
{
    t_frame *f;

    f = ring_frame_mut(k);
    if (!f)
        return;
    /* assert slot is free before writing */
    if (ring_slot_refs(k) != 0)
        return;
    fill_frame(f, step);
    ring_publish(k);
}

void* producer(void *arg)
{
    int k;
    int step;
    t_frame0 topo;
    t_frame *f;

    (void)arg;
    k = 0;
    step = 0;
    /* publish initial topology based on first frame */
    ring_wait_slot_free(k);
    f = ring_frame_mut(k);
    if (f) {
        fill_frame(f, step);
        topo_build_from_traj(&topo, f);
        topo_publish_global(&topo);
        ring_publish(k);
        k = next_k(k);
        step++;
    }
    while (1)
    {
        ring_wait_slot_free(k);
        produce_once(k, step);
        k = next_k(k);
        step++;
        sleep_ms(150);
    }
    return NULL;
}
