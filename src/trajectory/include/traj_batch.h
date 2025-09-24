/* traj_ring.h */
#pragma once
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "traj.h"
#include "xdrmini.h"

// api
int traj_open_auto(traj_ring *B, const char *path);
void traj_close_auto(traj_ring *B);
int traj_ring_init(traj_ring *R, int cap, const char *path);
int traj_ring_read_next(traj_ring *R);
void traj_ring_free(traj_ring *R);
