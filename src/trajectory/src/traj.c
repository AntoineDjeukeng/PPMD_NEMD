#include "traj.h"
#include "traj_batch.h"
#include <stdlib.h>
#include <string.h>

// static int ends_with(const char *s, const char *suf){
//     if(!s||!suf) return 0; 
//     size_t ls=strlen(s), lt=strlen(suf); if(ls<lt) return 0; 
//     return strcmp(s+ls-lt, suf)==0;
// }

// int traj_open_auto(traj_ring *B, const char *path)
// {
//     if(ends_with(path,".xtc")||ends_with(path,".XTC")) 
//     {
//         B->kind = TRAJ_KIND_XTC;
//         return xtc_open(B,path);
//     }
//     if(ends_with(path,".trr")||ends_with(path,".TRR"))
//     {
//         B->kind = TRAJ_KIND_TRR; 
//         return trr_open(B,path);
//     }
//     return -1;
// }

// int traj_read_frame(t_traj *t)
// {
//     if(!t) return exdrHEADER;
//     if(t->kind==TRAJ_KIND_XTC) return xtc_read_next(t);
//     if(t->kind==TRAJ_KIND_TRR) return trr_read_next(t);
//     return exdrHEADER;
// }

// void traj_close(t_traj *t)
// {
//     if(!t) return;
//     if(t->kind==TRAJ_KIND_XTC) xtc_close(t);
//     else if(t->kind==TRAJ_KIND_TRR) trr_close(t);
// }

// int traj_read_batch(t_traj *t, int n, t_batch *batch)
// {
//     if(!t||!batch||n<=0) 
//         return exdrINT;
//     memset(batch,0,sizeof(*batch));
//     for(int i=0;i<n;i++){
//         int rc=traj_read_frame(t);
//         if(rc==exdrENDOFFILE) break;
//         if(rc!=exdrOK) return rc;
//         const t_frame *fr=traj_tail(t); if(!fr) break;
//         batch->frames=(t_frame*)realloc(batch->frames, sizeof(t_frame)*(size_t)(i+1));
//         t_frame *dst=&batch->frames[i]; *dst=*fr;
//         if(fr->x && fr->natoms>0){
//             dst->x=(rvec*)malloc(sizeof(rvec)*(size_t)fr->natoms);
//             if(!dst->x) return exdrNOMEM;
//             memcpy(dst->x, fr->x, sizeof(rvec)*(size_t)fr->natoms);
//         } else dst->x=NULL;
//         if(batch->natoms==0) batch->natoms=fr->natoms;
//         batch->n=i+1;
//     }
//     return (batch->n>0)?exdrOK:exdrENDOFFILE;
// }

// void batch_free(t_batch *b)
// {
//     if(!b) return;
//     for(int i=0;i<b->n;i++) free(b->frames[i].x);
//     free(b->frames); b->frames=NULL; b->n=0; b->natoms=0;
// }

// int traj_next_velocity(t_traj *t, rvec *v_out, float *dt_ps, float *t_mid_ps)
// {
//     if(!t||!v_out) return exdrINT;
//     int rc=traj_read_frame(t); if(rc!=exdrOK) return rc;
//     const t_frame *a=traj_tail(t);
//     rc=traj_read_frame(t); if(rc!=exdrOK) return rc;
//     const t_frame *b=traj_tail(t);
//     if(!a||!b || a->natoms!=b->natoms) return exdrHEADER;
//     float dt=b->time - a->time; if(dt==0.0f) dt=1e-9f;
//     for(int i=0;i<a->natoms;i++){
//         v_out[i][0]=(b->x[i][0]-a->x[i][0])/dt;
//         v_out[i][1]=(b->x[i][1]-a->x[i][1])/dt;
//         v_out[i][2]=(b->x[i][2]-a->x[i][2])/dt;
//     }
//     if(dt_ps) *dt_ps=dt;
//     if(t_mid_ps) *t_mid_ps=0.5f*(a->time+b->time);
//     return exdrOK;
// }
