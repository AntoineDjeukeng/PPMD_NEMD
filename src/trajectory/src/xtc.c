#define _POSIX_C_SOURCE 200809L
#include "traj.h"
#include "list.h"
#include "xdrmini.h"
#include "traj_batch.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef int mybool;
enum { FALSE, TRUE };
typedef float matrix[DIM][DIM];

static int sizeofint(int size){ unsigned int num=1; int bits=0; while(size>=(int)num && bits<32){ bits++; num<<=1; } return bits; }
static int sizeofints(int n, unsigned int sizes[]){ int i,num,num_of_bytes=1; unsigned int bytes[32]; bytes[0]=1; unsigned int num_of_bits=0, bytecnt, tmp; for(i=0;i<n;i++){ tmp=0; for(bytecnt=0; bytecnt<(unsigned)num_of_bytes; bytecnt++){ tmp = bytes[bytecnt]*sizes[i] + tmp; bytes[bytecnt]=tmp & 0xff; tmp >>= 8; } while(tmp){ bytes[bytecnt++]=tmp & 0xff; tmp >>= 8; } num_of_bytes=(int)bytecnt; } num=1; num_of_bytes--; while(bytes[num_of_bytes] >= (unsigned)num){ num_of_bits++; num*=2; } return (int)num_of_bits + num_of_bytes*8; }
static int decodebits(int buf[], int nbits){ int cnt=buf[0]; unsigned int lastbits=(unsigned)buf[1], lastbyte=(unsigned)buf[2]; unsigned char *cbuf=((unsigned char*)buf)+3*sizeof(*buf); int mask=(1<<nbits)-1, num=0; while(nbits>=8){ lastbyte=(lastbyte<<8)|cbuf[cnt++]; num |= (int)((lastbyte>>lastbits) << (nbits-8)); nbits -= 8; } if(nbits>0){ if(lastbits < (unsigned)nbits){ lastbits += 8; lastbyte = (lastbyte<<8) | cbuf[cnt++]; } lastbits -= (unsigned)nbits; num |= (int)((lastbyte >> lastbits) & ((1u<<nbits)-1)); } num &= mask; buf[0]=cnt; buf[1]=(int)lastbits; buf[2]=(int)lastbyte; return num; }
static void decodeints(int buf[], int n, int nbits, unsigned int sizes[], int nums[]){ int bytes[32], i, j, num_of_bytes=0, p, num; bytes[1]=bytes[2]=bytes[3]=0; while(nbits>8){ bytes[num_of_bytes++] = decodebits(buf, 8); nbits -= 8; } if(nbits>0) bytes[num_of_bytes++] = decodebits(buf, nbits); for(i=n-1;i>0;i--){ num=0; for(j=num_of_bytes-1;j>=0;j--){ num = (num<<8) | bytes[j]; p = num / (int)sizes[i]; bytes[j]=p; num = num - p*(int)sizes[i]; } nums[i]=num; } nums[0] = bytes[0] | (bytes[1]<<8) | (bytes[2]<<16) | (bytes[3]<<24); }
static const int magicints[] = { 0,0,0,0,0,0,0,0,0, 8,10,12,16,20,25,32,40,50,64, 80,101,128,161,203,256,322,406,512,645,812,1024,1290, 1625,2048,2580,3250,4096,5060,6501,8192,10321,13003, 16384,20642,26007,32768,41285,52015,65536,82570,104031, 131072,165140,208063,262144,330280,416127,524287,660561, 832255,1048576,1321122,1664510,2097152,2642245,3329021, 4194304,5284491,6658042,8388607,10568983,13316085,16777216 };
#define FIRSTIDX 9

static int xdrfile_decompress_coord_float(float *ptr, int *size, float *precision, XDRFILE *xfp)
{
    int minint[3], maxint[3], *lip;
    int smallidx; unsigned sizeint[3], sizesmall[3], bitsizeint[3];
    int size3;
    int k, *buf1, *buf2, lsize;
    int smallnum, smaller, i, run;
    float *lfp, inv_precision;
    int tmp, prevcoord[3];
    unsigned int bitsize;
    bitsizeint[0]=bitsizeint[1]=bitsizeint[2]=0; if (xfp==NULL || ptr==NULL) return -1; if (xdrfile_read_int(&lsize,1,xfp)==0) return -1; if (*size < lsize) return -1;
    *size = lsize; size3 = *size * 3;
    if (size3 > xdrfile_buf1size(xfp)){ int *nb1=(int*)malloc(sizeof(int)*size3); if(!nb1) return -1; xdrfile_set_buf1(xfp, nb1, size3); int b2sz=(int)(size3*1.2); int *nb2=(int*)malloc(sizeof(int)*b2sz); if(!nb2) return -1; xdrfile_set_buf2(xfp, nb2, b2sz); }
    if (*size <= 9) return xdrfile_read_float(ptr, size3, xfp) / 3;
    xdrfile_read_float(precision, 1, xfp);
    buf1=xdrfile_buf1(xfp); buf2=xdrfile_buf2(xfp); buf2[0]=buf2[1]=buf2[2]=0; xdrfile_read_int(minint,3,xfp); xdrfile_read_int(maxint,3,xfp);
    sizeint[0]=maxint[0]-minint[0]+1; sizeint[1]=maxint[1]-minint[1]+1; sizeint[2]=maxint[2]-minint[2]+1;
    if ((sizeint[0]|sizeint[1]|sizeint[2]) > 0xffffff){ bitsizeint[0]=sizeofint(sizeint[0]); bitsizeint[1]=sizeofint(sizeint[1]); bitsizeint[2]=sizeofint(sizeint[2]); bitsize=0; }
    else { int sizesbits = sizeofints(3, sizeint); bitsize=(unsigned)sizesbits; }
    if (xdrfile_read_int(&smallidx,1,xfp)==0) return 0;
    tmp = smallidx - 1; tmp = (FIRSTIDX > (unsigned)tmp) ? (int)FIRSTIDX : tmp; smaller = magicints[tmp] / 2; smallnum = magicints[smallidx] / 2; sizesmall[0]=sizesmall[1]=sizesmall[2]=magicints[smallidx];
    if (xdrfile_read_int(buf2,1,xfp)==0) return 0;
    if (xdrfile_read_opaque((char*)&(buf2[3]), (unsigned)buf2[0], xfp)==0) return 0;
    buf2[0]=buf2[1]=buf2[2]=0;
    lfp=ptr; inv_precision=1.0f/(*precision); run=0; i=0; lip=buf1;
    while (i < lsize){
        int *thiscoord=(int*)lip + i*3;
        if (bitsize==0){ thiscoord[0]=decodebits(buf2,bitsizeint[0]); thiscoord[1]=decodebits(buf2,bitsizeint[1]); thiscoord[2]=decodebits(buf2,bitsizeint[2]); }
        else { decodeints(buf2,3,(int)bitsize,sizeint,thiscoord); }
        i++; thiscoord[0]+=minint[0]; thiscoord[1]+=minint[1]; thiscoord[2]+=minint[2]; prevcoord[0]=thiscoord[0]; prevcoord[1]=thiscoord[1]; prevcoord[2]=thiscoord[2];
        int flag=decodebits(buf2,1); int is_smaller=0; if (flag==1){ run=decodebits(buf2,5); is_smaller = run % 3; run -= is_smaller; is_smaller--; }
        if (run>0){
            thiscoord += 3;
            for (k=0; k<run; k+=3){
                decodeints(buf2,3,smallidx,sizesmall,thiscoord); i++;
                thiscoord[0]+=prevcoord[0]-smallnum; thiscoord[1]+=prevcoord[1]-smallnum; thiscoord[2]+=prevcoord[2]-smallnum;
                if (k==0){ int t; t=thiscoord[0]; thiscoord[0]=prevcoord[0]; prevcoord[0]=t; t=thiscoord[1]; thiscoord[1]=prevcoord[1]; prevcoord[1]=t; t=thiscoord[2]; thiscoord[2]=prevcoord[2]; prevcoord[2]=t; *lfp++=prevcoord[0]*inv_precision; *lfp++=prevcoord[1]*inv_precision; *lfp++=prevcoord[2]*inv_precision; }
                else { prevcoord[0]=thiscoord[0]; prevcoord[1]=thiscoord[1]; prevcoord[2]=thiscoord[2]; }
                *lfp++=thiscoord[0]*inv_precision; *lfp++=thiscoord[1]*inv_precision; *lfp++=thiscoord[2]*inv_precision;
            }
        } else {
            *lfp++=thiscoord[0]*inv_precision; *lfp++=thiscoord[1]*inv_precision; *lfp++=thiscoord[2]*inv_precision;
        }
        smallidx += is_smaller;
        if (is_smaller<0){ smallnum=smaller; if (smallidx>(int)FIRSTIDX) smaller = magicints[smallidx-1]/2; else smaller = 0; }
        else if (is_smaller>0){ smaller=smallnum; smallnum = magicints[smallidx]/2; }
    }
    return *size;
}

static int xtc_header(XDRFILE *xd, int *natoms, int *step, float *time, int bRead)
{
    int result, magic=1995, n=1;
    result = xdrfile_read_int(&magic,n,xd); if(result!=n) return bRead?exdrENDOFFILE:exdrINT;
    if(magic!=1995) return exdrMAGIC;
    if(xdrfile_read_int(natoms,n,xd)!=n) return exdrINT;
    if(xdrfile_read_int(step,n,xd)!=n) return exdrINT;
    if(xdrfile_read_float(time,n,xd)!=n) return exdrFLOAT;
    return exdrOK;
}

static int xtc_coord(XDRFILE *xd, int *natoms, matrix box, rvec *x, float *prec, int bRead)
{
    if(xdrfile_read_float(box[0], DIM*DIM, xd)!=DIM*DIM) return exdrFLOAT;
    if(bRead){
        int result = xdrfile_decompress_coord_float(x[0], natoms, prec, xd);
        if(result!=*natoms) return exdr3DX;
    } else return exdr3DX;
    return exdrOK;
}






// static int read_xtc(XDRFILE *xd, t_frame *fr)
// {
//     int natoms=fr->natoms, step=0; float time=0.0f;
//     int rc = xtc_header(xd,&natoms,&step,&time,1); if(rc!=exdrOK) return rc;
//     fr->step=step; fr->time=time; fr->natoms=natoms;
//     rc = xtc_coord(xd,&natoms,fr->box,fr->x,&fr->prec,1); if(rc!=exdrOK) return rc;
//     return exdrOK;
// }

// static int read_xtc_natoms(const char *fn, int *natoms)
// {
//     XDRFILE *xd = xdrfile_open(fn,"r"); int step, rc; float time;
//     if(!xd) return exdrFILENOTFOUND;
//     rc = xtc_header(xd, natoms, &step, &time, 1);
//     xdrfile_close(xd);
//     return rc;
// }


// int xtc_open(t_traj *t, const char *path, int cap)
// {
//     memset(t,0,sizeof(*t)); t->cap=(cap>=1)?cap:1; t->kind=TRAJ_KIND_XTC;
//     int nat=0; int rc=read_xtc_natoms(path,&nat); if(rc!=exdrOK||nat<=0) return rc==exdrOK?exdrHEADER:rc;
//     XtcCtx *ctx=(XtcCtx*)calloc(1,sizeof(*ctx)); if(!ctx) return exdrNOMEM;
//     ctx->xd=xdrfile_open(path,"r"); if(!ctx->xd){ free(ctx); return exdrFILENOTFOUND; }
//     ctx->natoms=nat; ctx->xbuf=(rvec*)malloc(sizeof(rvec)*(size_t)nat); if(!ctx->xbuf){ xdrfile_close(ctx->xd); free(ctx); return exdrNOMEM; }
//     t->ctx=ctx; t->natoms=nat; return exdrOK;
// }


int xtc_open(traj_ring *B, const char *path)
{
    if (!B || !path) return exdrHEADER;
    XDRFILE *xd = xdrfile_open(path, "r");
    if (!xd) return exdrFILENOTFOUND;
    int   natoms = 0;
    int   step   = 0;
    float time   = 0.0f;

    int rc = xtc_header(xd, &natoms, &step, &time, 1);
    xdrfile_close(xd);

    if (rc != exdrOK || natoms <= 0)
        return (rc == exdrOK) ? exdrHEADER : rc;

    xd = xdrfile_open(path, "r");
    if (!xd) return exdrFILENOTFOUND;
    memset(B, 0, sizeof(*B));
    B->kind   = TRAJ_KIND_XTC;
    B->xd     = xd;
    B->natoms = natoms;

    return exdrOK;
}




void xtc_close(traj_ring *B)
{
    if (!B) return;
    if (B->xd) xdrfile_close(B->xd);
}
int read_xtc_next(XDRFILE *xd, t_frame *fr)
{
    if (!xd || !fr || fr->natoms <= 0 || !fr->x) return exdrHEADER;

    int   step   = 0;
    float time   = 0.0f;
    int   natoms = fr->natoms;

    int rc = xtc_header(xd, &natoms, &step, &time, 1);
    if (rc == exdrENDOFFILE) return exdrENDOFFILE;
    if (rc != exdrOK)        return rc;

    if (natoms != fr->natoms) return exdrHEADER;

    float box[3][3];
    float prec = 0.0f;

    rc = xtc_coord(xd, &natoms, box, fr->x, &prec, 1);
    if (rc == exdrENDOFFILE) return exdrENDOFFILE;
    if (rc != exdrOK)        return rc;

    fr->step = step;
    fr->time = time;
    fr->prec = prec;
    memcpy(fr->box, box, sizeof(box));
    return exdrOK;
}