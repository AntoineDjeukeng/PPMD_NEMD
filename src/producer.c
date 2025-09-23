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
    char **av = (char **)arg;
    FILE *fp;
	t_topo f0;
	int rc;
    const char *json;
    t_topo topo;
    t_frame *f;

    
	json = "data/channel.json";
	fp = fopen(av[1], "r");
	if (!fp) {
		perror("fopen");
		return NULL;
	}
	rc = gro_parse(fp, &f0);
	fclose(fp);
	if (rc == -2) {
		fprintf(stderr,"Error: inconsistent atoms-per-molecule.\n");
		gro_free(&f0);
		return NULL;
	}
	if (rc != 0) {
		fprintf(stderr,"Parse error.\n");
		gro_free(&f0);
		return NULL;
	}
	if (gro_channel_update_or_load(&f0, json) != 0) {
		fprintf(stderr,"Note: no KIND_OTHER detected and no channel file found.\n");
	}
	// gro_print_essentials(&f0);
	// if (f0.sum.chan.axis != -1) {
	// 	if (gro_channel_save(&f0, json) == 0) {
	// 		fprintf(stderr, "Saved channel to %s\n", json);
	// 	}
	// }




    k = 0;
    step = 0;
    /* publish initial topology based on first frame */
    ring_wait_slot_free(k);
    f = ring_frame_mut(k);
    if (f) {
        fill_frame(f, step);
        topo_build_from_traj(&topo, f);
        topo_publish_global(&f0);
        ring_publish(k);
        k = next_k(k);
        step++;
    }
    int i = 0;
    while (1)
    {
        i++;
        ring_wait_slot_free(k);
        produce_once(k, step);
        k = next_k(k);
        step++;
        sleep_ms(150);
        if (i == 2) break;
    }

    gro_free(&f0);
    return NULL;
}
