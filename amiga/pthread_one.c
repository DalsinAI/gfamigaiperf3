/* pthread.h's calls for one task (see include/pthread.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <errno.h>
#include "pthread.h"

int gfip_threads_asked;

int pthread_create(pthread_t *t, const pthread_attr_t *a, void *(*fn)(void *), void *arg)
{
	(void)a; (void)fn; (void)arg;
	/* nothing starts: the stream is served by iperf_onetask_select() */
	*t = ++gfip_threads_asked;
	return 0;
}

int pthread_cancel(pthread_t t) { (void)t; return 0; }
int pthread_join(pthread_t t, void **ret) { (void)t; if (ret) *ret = 0; return 0; }
int pthread_attr_init(pthread_attr_t *a) { *a = 0; return 0; }
int pthread_attr_destroy(pthread_attr_t *a) { (void)a; return 0; }
int pthread_setcanceltype(int type, int *old) { (void)type; if (old) *old = 0; return 0; }
int pthread_setcancelstate(int state, int *old) { (void)state; if (old) *old = 0; return 0; }
int pthread_sigmask(int how, const sigset_t *set, sigset_t *old) { (void)how; (void)set; (void)old; return 0; }
int pthread_mutexattr_init(pthread_mutexattr_t *a) { *a = PTHREAD_MUTEX_NORMAL; return 0; }
int pthread_mutexattr_destroy(pthread_mutexattr_t *a) { (void)a; return 0; }

int pthread_mutexattr_settype(pthread_mutexattr_t *a, int type)
{
	if (type != PTHREAD_MUTEX_NORMAL && type != PTHREAD_MUTEX_ERRORCHECK) return EINVAL;
	*a = type;
	return 0;
}

int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a)
{
	m->type = a ? *a : PTHREAD_MUTEX_NORMAL;
	m->depth = 0;
	return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *m)
{
	return (m->type == PTHREAD_MUTEX_ERRORCHECK && m->depth) ? EBUSY : 0;
}

int pthread_mutex_lock(pthread_mutex_t *m)
{
	/* one task: a second lock without an unlock is the deadlock a real
	   error-checking mutex reports */
	if (m->type == PTHREAD_MUTEX_ERRORCHECK && m->depth) return EDEADLK;
	m->depth++;
	return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *m)
{
	if (!m->depth) return m->type == PTHREAD_MUTEX_ERRORCHECK ? EPERM : 0;
	m->depth--;
	return 0;
}
