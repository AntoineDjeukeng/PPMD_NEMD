#pragma once
#include <stdio.h>

typedef struct XDRFILE XDRFILE;

XDRFILE *xdrfile_open(const char *path, const char *mode);
int      xdrfile_close(XDRFILE *xfp);

int      xdrfile_read_int(int *ptr,int ndata,XDRFILE *xfp);
int      xdrfile_read_float(float *ptr,int ndata,XDRFILE *xfp);
int      xdrfile_read_double(double *ptr,int ndata,XDRFILE *xfp);
int      xdrfile_read_opaque(char *ptr,int nbytes,XDRFILE *xfp);
int      xdrfile_skip_bytes(XDRFILE *xfp, long long n);

int     *xdrfile_buf1(XDRFILE *xfp);
int     *xdrfile_buf2(XDRFILE *xfp);
int      xdrfile_buf1size(XDRFILE *xfp);
int      xdrfile_buf2size(XDRFILE *xfp);
void     xdrfile_set_buf1(XDRFILE *xfp, int *p, int sz);
void     xdrfile_set_buf2(XDRFILE *xfp, int *p, int sz);
long     xdrfile_tell(XDRFILE *xfp);
int      xdrfile_seek_set(XDRFILE *xfp, long pos);
