#include "pc.h"

static void split_range(int total, int parts, int idx, int *start, int *count)
{
    int base;
    int rem;
    int s;
    int c;

    if (total < 0)
        total = 0;
    if (parts < 1)
        parts = 1;
    if (idx < 0)
        idx = 0;
    if (idx >= parts)
        idx = parts - 1;
    base = total / parts;
    rem = total % parts;
    c = base + (idx < rem ? 1 : 0);
    s = idx * base + (idx < rem ? idx : rem);
    *start = s;
    *count = c;
}

static void consume_once_subset(int start, int count, const t_frame0 *topo, unsigned *last_gen)
{
    int k;
    const t_frame *f;
    unsigned my_gen;
    unsigned expected_prev;
    int pk;

    ring_wait_new_gen(&k, &my_gen);
    expected_prev = (my_gen > 0) ? (my_gen - 1) : 0;
    if (*last_gen + 1 == expected_prev) {
        pk = ring_slot_for_gen(expected_prev);
        f = ring_frame(pk);
        if (f) {
            process_frame_subset(f, topo, start, count);
            ring_ack(pk);
            *last_gen = expected_prev;
        }
    }
    /* ensure slot really holds this generation */
    if (ring_slot_gen(k) != my_gen) {
        k = ring_slot_for_gen(my_gen);
    }
    f = ring_frame(k);
    if (f) {
        process_frame_subset(f, topo, start, count);
        ring_ack(k);
        *last_gen = my_gen;
    }
}

void* consumer(void *arg)
{
    int idx;
    t_frame0 topo;
    int start;
    int count;
    unsigned seen_gen;

    idx = arg ? *(int*)arg : 0;
    topo_wait_global_copy(&topo);
    split_range(topo.natoms, NUM_CONSUMERS, idx, &start, &count);
    seen_gen = 0;
    while (1) {
        consume_once_subset(start, count, &topo, &seen_gen);
        sleep_ms(50);
    }
    return NULL;
}
