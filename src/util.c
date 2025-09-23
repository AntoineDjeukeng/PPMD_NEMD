#include "pc.h"
#include <time.h>
#include <stdio.h>

void sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

void set_stdout_line_buffered(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
}

