/* iperf's test streams, served from its own task.
 *
 * iperf 3.16 and later give each stream a thread that loops on
 * iperf_send_mt() or iperf_recv_mt() until the test is done. On GFOS a
 * socket belongs to the task that opened bsdsocket.library, and every
 * bsdsocket call goes through that task's library base, so the streams
 * stay in iperf's task: include/pthread.h starts no thread, and the main
 * loops' select() (patches/0001) is this function, which adds the running
 * streams to the wait and does one round of the thread's loop for each
 * that is ready:
 *   - a sender whose bit rate allows more: when its socket can be written,
 *     iperf_send_mt() (a burst of up to --burst or iperf's multisend
 *     messages, as the thread did);
 *   - a sender held back by --bitrate: iperf_send_mt() checks the rate
 *     again each round, and the wait is cut to IPERF_ONETASK_THROTTLE_USECS
 *     so it is checked often (in place of the thread's nanosleep);
 *   - a receiver: when its socket has data, iperf_recv_mt().
 * A stream is served while the thread would have run: from pthread_create
 * (thread_created 1) until the test or the stream is done or the stream is
 * cancelled (iperf sets thread_created back to 0).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "iperf_config.h"
#include <errno.h>
#include <string.h>
#include "iperf.h"
#include "iperf_api.h"
#include "iperf_onetask.h"

static int running(struct iperf_test *test, struct iperf_stream *sp)
{
    return sp->thread_created == 1 && !test->done && !sp->done && sp->socket >= 0;
}

int iperf_onetask_select(struct iperf_test *test, int nfds, fd_set *r, fd_set *w,
                         fd_set *e, struct timeval *timeout)
{
    struct iperf_stream *sp;
    struct timeval cap, *t = timeout;
    int maxfd = nfds - 1, held = 0, n;

    SLIST_FOREACH(sp, &test->streams, streams) {
        if (!running(test, sp))
            continue;
        if (sp->sender) {
            if (!w)
                continue;
            if (sp->green_light)
                FD_SET(sp->socket, w);
            else {
                held = 1;
                continue;
            }
        } else
            FD_SET(sp->socket, r);
        if (sp->socket > maxfd)
            maxfd = sp->socket;
    }
    if (held) {
        cap.tv_sec = 0;
        cap.tv_usec = IPERF_ONETASK_THROTTLE_USECS;
        if (!t || t->tv_sec > 0 || t->tv_usec > cap.tv_usec)
            t = &cap;
    }

    n = select(maxfd + 1, r, w, e, t);
    if (n < 0)
        return n;

    SLIST_FOREACH(sp, &test->streams, streams) {
        if (!running(test, sp))
            continue;
        if (sp->sender) {
            if (w && sp->green_light && FD_ISSET(sp->socket, w)) {
                FD_CLR(sp->socket, w);
                n--;
                if (iperf_send_mt(sp) < 0)
                    sp->thread_created = IPERF_ONETASK_FAILED;
            } else if (!sp->green_light) {
                /* sends nothing; checks the rate, which may give the light */
                if (iperf_send_mt(sp) < 0)
                    sp->thread_created = IPERF_ONETASK_FAILED;
            }
        } else if (FD_ISSET(sp->socket, r)) {
            FD_CLR(sp->socket, r);
            n--;
            if (iperf_recv_mt(sp) < 0)
                sp->thread_created = IPERF_ONETASK_FAILED;
        }
    }
    return n;
}
