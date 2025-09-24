#define _POSIX_C_SOURCE 200809L
#include "xdrmini.h"
#include "traj.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

enum xdr_op { XDR_ENCODE = 0, XDR_DECODE = 1, XDR_FREE = 2 };

typedef struct XDR XDR;
struct xdr_ops {
    int         (*x_getlong)  (XDR *, int32_t *);
    int         (*x_putlong)  (XDR *, int32_t *);
    int         (*x_getbytes) (XDR *, char *, unsigned int);
    int         (*x_putbytes) (XDR *, char *, unsigned int);
    unsigned int(*x_getpostn) (XDR *);
    int         (*x_setpostn) (XDR *, unsigned int);
    void        (*x_destroy)  (XDR *);
};
struct XDR {
    enum xdr_op    x_op;
    struct xdr_ops *x_ops;
    char          *x_private;
};

struct XDRFILE {
    FILE *fp;
    XDR  *xdr;
    char  mode;
    int  *buf1;
    int   buf1size;
    int  *buf2;
    int   buf2size;
} ;

static inline bool host_is_big_endian(void)
{
    uint16_t x = 0x0102;
    return *(uint8_t *)&x == 0x01;
}
static inline void swap_u64(uint8_t *p)
{
    uint8_t t; t=p[0]; p[0]=p[7]; p[7]=t; t=p[1]; p[1]=p[6]; p[6]=t; t=p[2]; p[2]=p[5]; p[5]=t; t=p[3]; p[3]=p[4]; p[4]=t;
}


static int32_t xdr_swapbytes(int32_t x){ int32_t y; char *px=(char*)&x,*py=(char*)&y; py[0]=px[3];py[1]=px[2];py[2]=px[1];py[3]=px[0]; return y; }
static int32_t xdr_htonl(int32_t x){ int s=0x1234; return (*((char*)&s)==(char)0x34)?xdr_swapbytes(x):x; }
static int32_t xdr_ntohl(int x){ int s=0x1234; return (*((char*)&s)==(char)0x34)?xdr_swapbytes(x):x; }

static int xdrstdio_getlong(XDR *xdrs, int32_t *lp){ int32_t tmp; if(fread(&tmp,4,1,(FILE*)xdrs->x_private)!=1) return 0; *lp=xdr_ntohl(tmp); return 1; }
static int xdrstdio_putlong(XDR *xdrs, int32_t *lp){ int32_t tmp=xdr_htonl(*lp); return fwrite(&tmp,4,1,(FILE*)xdrs->x_private)==1; }
static int xdrstdio_getbytes(XDR *xdrs, char *addr, unsigned int len){ return (len==0)||fread(addr,(int)len,1,(FILE*)xdrs->x_private)==1; }
static int xdrstdio_putbytes(XDR *xdrs, char *addr, unsigned int len){ return (len==0)||fwrite(addr,(int)len,1,(FILE*)xdrs->x_private)==1; }
static unsigned int xdrstdio_getpos(XDR *xdrs){ return (unsigned int)ftell((FILE*)xdrs->x_private); }
static int xdrstdio_setpos(XDR *xdrs, unsigned int pos){ return fseek((FILE*)xdrs->x_private,(long)pos,SEEK_SET)<0?0:1; }
static void xdrstdio_destroy(XDR *xdrs){ (void)fflush((FILE*)xdrs->x_private); }

static struct xdr_ops xdrstdio_ops = {
    xdrstdio_getlong, xdrstdio_putlong, xdrstdio_getbytes, xdrstdio_putbytes,
    xdrstdio_getpos, xdrstdio_setpos, xdrstdio_destroy
};

static void xdrstdio_create(XDR *xdrs, FILE *file, enum xdr_op op){
    xdrs->x_op=op; xdrs->x_ops=&xdrstdio_ops; xdrs->x_private=(char*)file;
}

XDRFILE *xdrfile_open(const char *path, const char *mode)
{
    char newmode[5]; enum xdr_op xdrmode; XDRFILE *xfp;
    if (*mode=='w'||*mode=='W'){ sprintf(newmode,"wb+"); xdrmode=XDR_ENCODE; }
    else if (*mode=='a'||*mode=='A'){ sprintf(newmode,"ab+"); xdrmode=XDR_ENCODE; }
    else if (*mode=='r'||*mode=='R'){ sprintf(newmode,"rb"); xdrmode=XDR_DECODE; }
    else return NULL;
    xfp=(XDRFILE*)calloc(1,sizeof(*xfp)); if(!xfp) return NULL;
    xfp->fp=fopen(path,newmode); if(!xfp->fp){ free(xfp); return NULL; }
    xfp->xdr=(XDR*)malloc(sizeof(XDR)); if(!xfp->xdr){ fclose(xfp->fp); free(xfp); return NULL; }
    xdrstdio_create(xfp->xdr, xfp->fp, xdrmode);
    xfp->mode=*mode; xfp->buf1=xfp->buf2=NULL; xfp->buf1size=xfp->buf2size=0;
    return xfp;
}



int xdrfile_close(XDRFILE *xfp)
{
    int ret = 0;
    if(!xfp) return ret;
    free(xfp->xdr);
    ret = fclose(xfp->fp);
    if(xfp->buf1) free(xfp->buf1);
    if(xfp->buf2) free(xfp->buf2);
    free(xfp);
    return ret;
}

static int xdr_int(XDR *xdrs, int *ip)
{
    int32_t i32;
    if(xdrs->x_op==XDR_ENCODE){ i32=(int32_t)(*ip); return xdrstdio_putlong(xdrs,&i32); }
    if(xdrs->x_op==XDR_DECODE){ if(!xdrstdio_getlong(xdrs,&i32)) return 0; *ip=(int)i32; return 1; }
    return 1;
}

static int xdr_float(XDR *xdrs, float *fp)
{
    if(sizeof(float)==sizeof(int32_t)){
        if(xdrs->x_op==XDR_ENCODE) return xdrstdio_putlong(xdrs,(int32_t*)fp);
        if(xdrs->x_op==XDR_DECODE) return xdrstdio_getlong(xdrs,(int32_t*)fp);
        return 1;
    } else {
        int32_t tmp;
        if(xdrs->x_op==XDR_ENCODE){ tmp=*(int*)fp; return xdrstdio_putlong(xdrs,&tmp); }
        if(xdrs->x_op==XDR_DECODE){ if(xdrstdio_getlong(xdrs,&tmp)){ *(int*)fp=tmp; return 1; } return 0; }
        return 1;
    }
}

int xdrfile_read_int(int *ptr,int ndata,XDRFILE *xfp){ int i=0; while(i<ndata && xdr_int(xfp->xdr, ptr+i)) i++; return i; }
int xdrfile_read_float(float *ptr,int ndata,XDRFILE *xfp){ int i=0; while(i<ndata && xdr_float(xfp->xdr, ptr+i)) i++; return i; }


int xdrfile_read_double(double *ptr,int ndata,XDRFILE *xfp)
{
    /* XDR is big-endian. Read raw 8 bytes per double and swap on little-endian hosts. */
    int i=0;
    for(; i<ndata; ++i){
        uint8_t buf[8];
        if (fread(buf, 1, 8, xfp->fp) != 8) break;
        if (!host_is_big_endian()) swap_u64(buf);
        double d;
        memcpy(&d, buf, 8);
        ptr[i]=d;
    }
    return i;
}
static int xdr_opaque(XDR *xdrs, char *cp, unsigned int cnt)
{
    unsigned int rndup=cnt%4; if(rndup) rndup=4-rndup; static char zero[4]={0,0,0,0};
    if(xdrs->x_op==XDR_DECODE){ if(!xdrstdio_getbytes(xdrs,cp,cnt)) return 0; return rndup?xdrstdio_getbytes(xdrs,zero,rndup):1; }
    if(xdrs->x_op==XDR_ENCODE){ if(!xdrstdio_putbytes(xdrs,cp,cnt)) return 0; return rndup?xdrstdio_putbytes(xdrs,zero,rndup):1; }
    return 1;
}

int xdrfile_read_opaque(char *ptr,int nbytes,XDRFILE *xfp){ return xdr_opaque(xfp->xdr, ptr, (unsigned)nbytes) ? nbytes : 0; }

int xdrfile_skip_bytes(XDRFILE *xfp, long long n)
{
#if defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS==64)
    if(fseeko(xfp->fp,(off_t)n,SEEK_CUR)==0) return 1;
#else
    if(fseek(xfp->fp,(long)n,SEEK_CUR)==0) return 1;
#endif
    char buf[4096];
    while(n>0){
        size_t chunk=(size_t)((n<(long long)sizeof(buf))?n:(long long)sizeof(buf));
        size_t got=fread(buf,1,chunk,xfp->fp);
        if(got==0) return 0;
        n -= (long long)got;
    }
    return 1;
}

int *xdrfile_buf1(XDRFILE *xfp){ return xfp->buf1; }
int *xdrfile_buf2(XDRFILE *xfp){ return xfp->buf2; }
int  xdrfile_buf1size(XDRFILE *xfp){ return xfp->buf1size; }
int  xdrfile_buf2size(XDRFILE *xfp){ return xfp->buf2size; }
void xdrfile_set_buf1(XDRFILE *xfp, int *p, int sz){ xfp->buf1=p; xfp->buf1size=sz; }
void xdrfile_set_buf2(XDRFILE *xfp, int *p, int sz){ xfp->buf2=p; xfp->buf2size=sz; }




/* wrappers used by trr.c */
long xdrfile_tell(XDRFILE *xfp){ return ftell(xfp->fp); }
int  xdrfile_seek_set(XDRFILE *xfp, long pos){ return fseek(xfp->fp, pos, SEEK_SET)==0; }
