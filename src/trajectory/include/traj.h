#pragma once
#include <stddef.h>
enum { exdrOK, exdrHEADER, exdrSTRING, exdrDOUBLE,
       exdrINT, exdrFLOAT, exdrUINT, exdr3DX, exdrCLOSE, exdrMAGIC,
       exdrNOMEM, exdrENDOFFILE, exdrFILENOTFOUND, exdrNR };

#define DIM 3
typedef float rvec[DIM];
typedef struct XDRFILE XDRFILE;
typedef struct {
    int    natoms;
    int    step;
    float  time;
    float  box[3][3];
    rvec  *x;
    rvec  *v;
    rvec  *f;
    float  prec;
} t_frame;


typedef enum {
    TRAJ_KIND_UNKNOWN = 0,
    TRAJ_KIND_XTC     = 1,
    TRAJ_KIND_TRR     = 2,
} t_traj_kind;

typedef struct {
    int      natoms;
    int      cap;
    t_traj_kind kind;
    XDRFILE *xd;
    int      count;
    int      start;
    t_frame *slot;
} traj_ring;

typedef struct s_node {
    t_frame         fr;
    struct s_node  *next;
} t_node;


typedef struct {
    t_node     *head;
    t_node     *tail;
    int         size;
    int         cap;
    t_traj_kind kind;
    long long   nframes;
    int         natoms;
    void       *ctx;
} t_traj;

typedef struct {
    int       natoms;
    int       n;
    t_frame  *frames;
} t_batch;

typedef struct {
    XDRFILE  *xd;
    int       natoms;
    rvec     *xbuf;
} XtcCtx;







static inline const t_frame *traj_tail(const t_traj *t) { return t && t->tail ? &t->tail->fr : NULL; }
static inline const char *traj_kind_str(const t_traj *t) {
    switch (t ? t->kind : TRAJ_KIND_UNKNOWN) {
        case TRAJ_KIND_XTC: return "xtc";
        case TRAJ_KIND_TRR: return "trr";
        default:            return "unknown";
    }
}

int  traj_open_auto(traj_ring *B, const char *path);
int  traj_read_frame(t_traj *t);
void traj_close(t_traj *t);

int  traj_read_batch(t_traj *t, int n, t_batch *batch);
void batch_free(t_batch *batch);

int  xtc_open(traj_ring *B, const char *path);
int read_xtc_next(XDRFILE *xd, t_frame *fr);
void xtc_close(traj_ring *B);

int trr_open(traj_ring *B, const char *path);
int read_trr_next(XDRFILE *xd, t_frame *fr);
void trr_close(traj_ring *B);
