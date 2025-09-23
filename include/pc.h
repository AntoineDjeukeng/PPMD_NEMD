#ifndef PC_H
#define PC_H

#define _POSIX_C_SOURCE 199309L

#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include "gro.h"

/* configuration */
#define THREAD_COUNT 8
#define NUM_CONSUMERS (THREAD_COUNT - 1)
#define SLOTS 2

/* Trajectory data types */
typedef struct s_xtc_node XtcNode;
typedef enum e_traj_kind { TRAJ_KIND_UNKNOWN = 0 } t_traj_kind;

typedef struct s_xtc_traj {
    XtcNode    *head;
    XtcNode    *tail;
    int         size;
    int         cap;
    t_traj_kind kind;
    long long   nframes;
    int         natoms;
    void       *ctx;
} XtcTraj;




typedef XtcTraj t_frame;

/* compile-time checks */
_Static_assert(NUM_CONSUMERS > 0, "NUM_CONSUMERS must be > 0");
_Static_assert(SLOTS == 2, "SLOTS must be 2 for double-buffer mode");

/* ring API */
void        ring_init(void);
void        ring_wait_slot_free(int k);
void        ring_publish(int k);
void        ring_wait_new_gen(int *k, unsigned *gen_out);
const t_frame* ring_frame(int k);
t_frame*       ring_frame_mut(int k);
void        ring_ack(int k);
int         ring_slot_for_gen(unsigned gen);
unsigned    ring_slot_gen(int k);
int         ring_slot_refs(int k);

/* topology (from gro.h): t_topo */
void        topo_build_from_traj(t_topo *dst, const t_frame *src);
void        topo_copy(t_topo *dst, const t_topo *src);
void        topo_free(t_topo *t);
void        topo_publish_global(const t_topo *src);
void        topo_wait_global_copy(t_topo *dst);
int         topo_count_mols(const t_topo *t);

/* demo workload */
void        fill_frame(t_frame *f, int step);
void        process_frame(const t_frame *f);
void        process_frame_subset(const t_frame *f,
                                 const t_topo *topo,
                                 int start,
                                 int count);

/* util */
void        sleep_ms(int ms);
void        set_stdout_line_buffered(void);

/* threads */
void*       producer(void *arg);
void*       consumer(void *arg);

/* runtime config */

#endif
