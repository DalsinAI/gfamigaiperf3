/* The POSIX thread calls iperf makes, for one task.
 *
 * iperf 3.16 and later run each test stream in a thread of its own. On
 * GFOS a socket belongs to the task that opened bsdsocket.library, so the
 * streams stay in iperf's own task: pthread_create() starts nothing and
 * says it did, iperf marks the stream thread_created, and the main loop's
 * select() (amiga/iperf_onetask.c, patches/0001) sends or receives on
 * every such stream that is ready. Cancel and join have nothing to stop.
 * The mutex guards iperf's printing; in one task it only counts, so the
 * error-checking kind still reports an unlock without a lock.
 * Our own names, so the PC test's C library (which has its own) doesn't
 * clash. MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GFIPERF_PTHREAD_H
#define GFIPERF_PTHREAD_H
#include <signal.h>

#define pthread_t gfip_thread_t
#define pthread_attr_t gfip_attr_t
#define pthread_mutex_t gfip_mutex_t
#define pthread_mutexattr_t gfip_mutexattr_t
#define pthread_create gfip_create
#define pthread_cancel gfip_cancel
#define pthread_join gfip_join
#define pthread_attr_init gfip_attr_init
#define pthread_attr_destroy gfip_attr_destroy
#define pthread_setcanceltype gfip_setcanceltype
#define pthread_setcancelstate gfip_setcancelstate
#define pthread_sigmask gfip_sigmask
#define pthread_mutex_init gfip_mutex_init
#define pthread_mutex_destroy gfip_mutex_destroy
#define pthread_mutex_lock gfip_mutex_lock
#define pthread_mutex_unlock gfip_mutex_unlock
#define pthread_mutexattr_init gfip_mutexattr_init
#define pthread_mutexattr_destroy gfip_mutexattr_destroy
#define pthread_mutexattr_settype gfip_mutexattr_settype
#undef PTHREAD_MUTEX_ERRORCHECK
#undef PTHREAD_MUTEX_NORMAL
#undef PTHREAD_CANCEL_ENABLE
#undef PTHREAD_CANCEL_DISABLE
#undef PTHREAD_CANCEL_DEFERRED
#undef PTHREAD_CANCEL_ASYNCHRONOUS
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_CANCEL_ENABLE 0
#define PTHREAD_CANCEL_DISABLE 1
#define PTHREAD_CANCEL_DEFERRED 0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1

typedef int pthread_t;
typedef int pthread_attr_t;
typedef struct { int type, depth; } pthread_mutex_t;
typedef int pthread_mutexattr_t;

int pthread_create(pthread_t *t, const pthread_attr_t *a, void *(*fn)(void *), void *arg);
int pthread_cancel(pthread_t t);
int pthread_join(pthread_t t, void **ret);
int pthread_attr_init(pthread_attr_t *a);
int pthread_attr_destroy(pthread_attr_t *a);
int pthread_setcanceltype(int type, int *old);
int pthread_setcancelstate(int state, int *old);
int pthread_sigmask(int how, const sigset_t *set, sigset_t *old);
int pthread_mutexattr_init(pthread_mutexattr_t *a);
int pthread_mutexattr_destroy(pthread_mutexattr_t *a);
int pthread_mutexattr_settype(pthread_mutexattr_t *a, int type);
int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a);
int pthread_mutex_destroy(pthread_mutex_t *m);
int pthread_mutex_lock(pthread_mutex_t *m);
int pthread_mutex_unlock(pthread_mutex_t *m);

/* how many times pthread_create was asked for a thread (the tests read it) */
extern int gfip_threads_asked;
#endif
