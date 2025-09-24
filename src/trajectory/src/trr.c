#define _POSIX_C_SOURCE 200809L
#include "traj.h"
#include "xdrmini.h"
#include "traj_batch.h"
#include "list.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
    int32_t ir_size, e_size, box_size, vir_size, pres_size, top_size, sym_size;
    int32_t x_size, v_size, f_size;
    int32_t natoms, step, nre;
    int     real_size;
    double  time, lambda;
} trr_hdr_t;

static int detect_real_size_from(int natoms,int box,int x,int v,int f){
    if(box==9*8 || box==9*4) return box/9;
    if(natoms>0){
        if(x%(3*natoms)==0){ int s=x/(3*natoms); if(s==4||s==8) return s; }
        if(v%(3*natoms)==0){ int s=v/(3*natoms); if(s==4||s==8) return s; }
        if(f%(3*natoms)==0){ int s=f/(3*natoms); if(s==4||s==8) return s; }
    }
    return 4;
}
static int plausible_sizes(const trr_hdr_t *h){
    if(h->natoms<=0 || h->natoms>100000000) return 0;
    if(h->real_size!=4 && h->real_size!=8) return 0;
    int rs=h->real_size, need=3*h->natoms*rs;
    int any_vec = (h->x_size==need) || (h->v_size==need) || (h->f_size==need);
    int box_ok  = (h->box_size==0) || (h->box_size==9*rs);
    return any_vec || box_ok;
}

static int try_read_header_B(XDRFILE *xd, trr_hdr_t *h){
    long start = xdrfile_tell(xd);
    memset(h,0,sizeof(*h));
    int magic=0; if(xdrfile_read_int(&magic,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    if(magic!=1993){ xdrfile_seek_set(xd,start); return 0; }
    int vlen=0,slen=0;
    if(xdrfile_read_int(&vlen,1,xd)!=1 || xdrfile_read_int(&slen,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    if(slen<0 || slen>4096){ xdrfile_seek_set(xd,start); return 0; }
    if(slen>0){
        char *tmp=(char*)malloc((size_t)slen); if(!tmp){ xdrfile_seek_set(xd,start); return 0; }
        if(xdrfile_read_opaque(tmp,slen,xd)!=slen){ free(tmp); xdrfile_seek_set(xd,start); return 0; }
        free(tmp);
    }
    if(xdrfile_read_int(&h->ir_size,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->e_size ,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->box_size,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->vir_size,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->pres_size,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->top_size,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->sym_size,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->x_size,1,xd)!=1)  { xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->v_size,1,xd)!=1)  { xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->f_size,1,xd)!=1)  { xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->natoms,1,xd)!=1 || xdrfile_read_int(&h->step,1,xd)!=1 || xdrfile_read_int(&h->nre,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    h->real_size = detect_real_size_from(h->natoms,h->box_size,h->x_size,h->v_size,h->f_size);
    if(h->real_size==8){
        if(xdrfile_read_double(&h->time,1,xd)!=1 || xdrfile_read_double(&h->lambda,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    } else {
        float tf=0,lf=0; if(xdrfile_read_float(&tf,1,xd)!=1 || xdrfile_read_float(&lf,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
        h->time=tf; h->lambda=lf;
    }
    if(!plausible_sizes(h)){ xdrfile_seek_set(xd,start); return 0; }
    return 1;
}

static int try_read_header_A(XDRFILE *xd, trr_hdr_t *h){
    long start = xdrfile_tell(xd);
    memset(h,0,sizeof(*h));
    int magic=0; if(xdrfile_read_int(&magic,1,xd)!=1) return 0;
    if(magic!=1993){ xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->ir_size,1,xd)!=1 || xdrfile_read_int(&h->e_size,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    long tag_pos = xdrfile_tell(xd);
    char tag[12];
    if(xdrfile_read_opaque(tag,12,xd)!=12){ xdrfile_seek_set(xd,start); return 0; }
    if(memcmp(tag,"GMX_trn_file",12)!=0){ xdrfile_seek_set(xd,tag_pos); }
    if(xdrfile_read_int(&h->box_size,1,xd)!=1) { xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->vir_size,1,xd)!=1) { xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->pres_size,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->top_size,1,xd)!=1) { xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->sym_size,1,xd)!=1) { xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->x_size,1,xd)!=1)   { xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->v_size,1,xd)!=1)   { xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->f_size,1,xd)!=1)   { xdrfile_seek_set(xd,start); return 0; }
    if(xdrfile_read_int(&h->natoms,1,xd)!=1 || xdrfile_read_int(&h->step,1,xd)!=1 || xdrfile_read_int(&h->nre,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    h->real_size = detect_real_size_from(h->natoms,h->box_size,h->x_size,h->v_size,h->f_size);
    if(h->real_size==8){
        if(xdrfile_read_double(&h->time,1,xd)!=1 || xdrfile_read_double(&h->lambda,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
    } else {
        float tf=0,lf=0; if(xdrfile_read_float(&tf,1,xd)!=1 || xdrfile_read_float(&lf,1,xd)!=1){ xdrfile_seek_set(xd,start); return 0; }
        h->time=tf; h->lambda=lf;
    }
    if(!plausible_sizes(h)){ xdrfile_seek_set(xd,start); return 0; }
    return 1;
}

static int trr_read_header(XDRFILE *xd, trr_hdr_t *h){
    long pos = xdrfile_tell(xd);
    if(try_read_header_B(xd,h)) return 1;
    xdrfile_seek_set(xd,pos);
    if(try_read_header_A(xd,h)) return 1;
    xdrfile_seek_set(xd,pos);
    return 0;
}

static int read_box_payload(XDRFILE *xd,int real_size,float B[3][3]){
    if(real_size==8){
        double b[9]; if(xdrfile_read_double(b,9,xd)!=9) return 0;
        for(int r=0,k=0;r<3;r++) for(int c=0;c<3;c++,k++) B[r][c]=(float)b[k];
    } else {
        float b[9]; if(xdrfile_read_float(b,9,xd)!=9) return 0;
        for(int r=0,k=0;r<3;r++) for(int c=0;c<3;c++,k++) B[r][c]=b[k];
    }
    return 1;
}

static int read_vec_block(XDRFILE *xd,int natoms,int real_size,float **out){
    int n3=3*natoms; if(n3<=0){ *out=NULL; return 1; }
    float *buf=(float*)malloc((size_t)n3*sizeof(float)); if(!buf) return 0;
    if(real_size==8){
        double *tmp=(double*)malloc((size_t)n3*sizeof(double)); if(!tmp){ free(buf); return 0; }
        if(xdrfile_read_double(tmp,n3,xd)!=n3){ free(tmp); free(buf); return 0; }
        for(int i=0;i<n3;i++) buf[i]=(float)tmp[i];
        free(tmp);
    } else {
        if(xdrfile_read_float(buf,n3,xd)!=n3){ free(buf); return 0; }
    }
    *out=buf; return 1;
}



int trr_open(traj_ring *B, const char *path)
{
    if (!B || !path) return exdrHEADER;
    XDRFILE *xd = xdrfile_open(path, "r");
    if (!xd) return exdrFILENOTFOUND;
    trr_hdr_t h;
    int ok = trr_read_header(xd, &h);
    xdrfile_close(xd);
    if (!ok || h.natoms <= 0) return exdrHEADER;
    xd = xdrfile_open(path, "r");
    if (!xd) return exdrFILENOTFOUND;
    memset(B, 0, sizeof(*B));
    B->kind   = TRAJ_KIND_TRR;
    B->xd     = xd;
    B->natoms = h.natoms;

    return exdrOK;
}


int read_trr_next(XDRFILE *xd, t_frame *fr)
{
    if (!xd || !fr || fr->natoms <= 0 || !fr->x) return exdrHEADER;

    trr_hdr_t h;
    if (!trr_read_header(xd, &h)) return exdrENDOFFILE;

    /* Skip auxiliary chunks in file order */
    if (h.ir_size  > 0 && !xdrfile_skip_bytes(xd, h.ir_size))   return exdrHEADER;
    if (h.e_size   > 0 && !xdrfile_skip_bytes(xd, h.e_size))    return exdrHEADER;

    /* Box */
    float B[3][3] = {{0}};
    if (h.box_size > 0) {
        if (!read_box_payload(xd, h.real_size, B))              return exdrHEADER;
    }

    /* Skip remaining optional chunks */
    if (h.vir_size > 0 && !xdrfile_skip_bytes(xd, h.vir_size))  return exdrHEADER;
    if (h.pres_size> 0 && !xdrfile_skip_bytes(xd, h.pres_size)) return exdrHEADER;
    if (h.top_size > 0 && !xdrfile_skip_bytes(xd, h.top_size))  return exdrHEADER;
    if (h.sym_size > 0 && !xdrfile_skip_bytes(xd, h.sym_size))  return exdrHEADER;

    if (h.natoms != fr->natoms) return exdrHEADER;

    /* Payloads → temporary flat buffers (helpers allocate) */
    float *x_flat = NULL, *v_flat = NULL, *f_flat = NULL;
    if (h.x_size > 0 && !read_vec_block(xd, h.natoms, h.real_size, &x_flat)) goto err_io;
    if (h.v_size > 0 && !read_vec_block(xd, h.natoms, h.real_size, &v_flat)) goto err_io;
    if (h.f_size > 0 && !read_vec_block(xd, h.natoms, h.real_size, &f_flat)) goto err_io;

    /* Fill metadata */
    fr->step = h.step;
    fr->time = (float)h.time;
    memcpy(fr->box, B, sizeof(B));
    fr->prec = 0.0f;

    /* Copy vectors (discard if dest is NULL) */
    if (x_flat && fr->x) {
        for (int i = 0; i < h.natoms; ++i) {
            fr->x[i][0] = x_flat[3*i+0];
            fr->x[i][1] = x_flat[3*i+1];
            fr->x[i][2] = x_flat[3*i+2];
        }
    }
    if (fr->v) {
        if (v_flat) {
            for (int i = 0; i < h.natoms; ++i) {
                fr->v[i][0] = v_flat[3*i+0];
                fr->v[i][1] = v_flat[3*i+1];
                fr->v[i][2] = v_flat[3*i+2];
            }
        } else {
            memset(fr->v, 0, (size_t)h.natoms * sizeof(rvec));
        }
    }
    if (fr->f) {
        if (f_flat) {
            for (int i = 0; i < h.natoms; ++i) {
                fr->f[i][0] = f_flat[3*i+0];
                fr->f[i][1] = f_flat[3*i+1];
                fr->f[i][2] = f_flat[3*i+2];
            }
        } else {
            memset(fr->f, 0, (size_t)h.natoms * sizeof(rvec));
        }
    }

    free(x_flat); free(v_flat); free(f_flat);
    return exdrOK;

err_io:
    free(x_flat); free(v_flat); free(f_flat);
    return (errno == ENOMEM) ? exdrNOMEM : exdrHEADER;
}




void trr_close(traj_ring *B)
{

    if (!B) return;
    if (B->xd) xdrfile_close(B->xd);
}
