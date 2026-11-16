/* iperf's test streams served from its own task (see iperf_onetask.c).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GFIPERF_ONETASK_H
#define GFIPERF_ONETASK_H
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>

struct iperf_test;

/* select() for iperf's main loops: also waits on every running stream,
   sends or receives on those that are ready, and returns how many of the
   caller's own descriptors are ready (their bits are left in the sets) */
int iperf_onetask_select(struct iperf_test *test, int nfds, fd_set *r, fd_set *w,
                         fd_set *e, struct timeval *timeout);

/* a stream whose send or receive failed: left alone, as its thread would
   have ended (thread_created 1 is a running stream) */
#define IPERF_ONETASK_FAILED 2
/* longest wait while a sender waits for its bit rate to allow more */
#define IPERF_ONETASK_THROTTLE_USECS 1000
#endif
