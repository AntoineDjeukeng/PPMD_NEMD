#include "traj_batch.h"
int traj_ring_read_next(traj_ring *R)
{
    if (!R || !R->xd || R->cap <= 0) return exdrHEADER;

    t_frame *dst = &R->slot[R->start];
    int rc;
    if (R->kind == TRAJ_KIND_XTC)
        rc = read_xtc_next(R->xd, dst);
    else if (R->kind == TRAJ_KIND_TRR)
        rc = read_trr_next(R->xd, dst);
    else
        return exdrHEADER;

    if (rc == exdrOK) 
    {
        if (R->count < R->cap) 
            R->count += 1;
        R->start = (R->start + 1) % R->cap;
    }
    return rc;
}
static int ends_with(const char *s, const char *suf){
    if(!s||!suf) return 0; 
    size_t ls=strlen(s), lt=strlen(suf); if(ls<lt) return 0; 
    return strcmp(s+ls-lt, suf)==0;
}

int traj_open_auto(traj_ring *B, const char *path)
{
    if(ends_with(path,".xtc")||ends_with(path,".XTC")) 
    {
        B->kind = TRAJ_KIND_XTC;
        return xtc_open(B,path);
    }
    if(ends_with(path,".trr")||ends_with(path,".TRR"))
    {
        B->kind = TRAJ_KIND_TRR; 
        return trr_open(B,path);
    }
    return -1;
}

/* ======================= Implementation ======================= */
int traj_ring_init(traj_ring *R, int cap, const char *path)
{
    if (!R || cap <= 0 || !path) return -1;

    if(traj_open_auto(R, path)!=exdrOK) return -1;
    if(R->kind==TRAJ_KIND_UNKNOWN) return -1;
    R->cap    = cap;
    R->start  = 0;                   /* write cursor */
    R->count  = 0;

    R->slot = (t_frame*)calloc((size_t)cap, sizeof(*R->slot));
    if (!R->slot) { traj_ring_free(R); return -1; }

    for (int i = 0; i < cap; ++i) {
        t_frame *fr = &R->slot[i];
        fr->natoms  = R->natoms;

        fr->x = (rvec*)calloc((size_t)R->natoms, sizeof(rvec));
        if (!fr->x) { traj_ring_free(R); return -1; }

        if (R->kind == TRAJ_KIND_TRR) {
            fr->v = (rvec*)calloc((size_t)R->natoms, sizeof(rvec));
            fr->f = (rvec*)calloc((size_t)R->natoms, sizeof(rvec));
            if (!fr->v || !fr->f) { traj_ring_free(R); return -1; }
        }

        /* prefill up to cap-1; leave last slot allocated but empty */
        if (i < cap - 1) {
            int rc = traj_ring_read_next(R);    /* writes at start; advances on success */
            if (rc == exdrENDOFFILE) 
                break;         /* fine: done pre-filling */
            if (rc != exdrOK) {                     /* real I/O error */
                traj_ring_free(R);
                return -1;
            }
        }
    }
    return 0;
}


void traj_close_auto(traj_ring *B)
{
    if (!B) return;
    if (B->kind == TRAJ_KIND_XTC) xtc_close(B);
    else if (B->kind == TRAJ_KIND_TRR) trr_close(B);
}

void traj_ring_free(traj_ring *R)
{
    if (!R) return;
    if (R->xd) traj_close_auto(R);
    if (R->slot) {
        for (int i = 0; i < R->cap; ++i) {
            free(R->slot[i].x);
            free(R->slot[i].v);
            free(R->slot[i].f);
        }
        free(R->slot);
    }
    memset(R, 0, sizeof(*R));
}
