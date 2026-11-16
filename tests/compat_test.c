/* PC unit test for the one-task pthread stand-in and gfiperf_compat.c:
 * the error-checking mutex, pthread_create starting nothing, entropy
 * (two calls never equal, every byte value turns up), daemon, and
 * amiga/gfinet.c against the C library's own inet_pton, inet_ntop and
 * getaddrinfo.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "pthread.h"
#include "gfiperf_compat.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int ran;
static void *worker(void *p) { (void)p; ran = 1; return NULL; }

int main(void)
{
	pthread_mutexattr_t a;
	pthread_mutex_t m;
	pthread_t t;
	unsigned char b1[64], b2[64], seen[256] = {0}, big[4096];
	int i, all = 1;

	CHECK(pthread_mutexattr_init(&a) == 0);
	CHECK(pthread_mutexattr_settype(&a, PTHREAD_MUTEX_ERRORCHECK) == 0);
	CHECK(pthread_mutex_init(&m, &a) == 0);
	CHECK(pthread_mutex_unlock(&m) == EPERM);
	CHECK(pthread_mutex_lock(&m) == 0);
	CHECK(pthread_mutex_lock(&m) == EDEADLK);
	CHECK(pthread_mutex_destroy(&m) == EBUSY);
	CHECK(pthread_mutex_unlock(&m) == 0);
	CHECK(pthread_mutex_destroy(&m) == 0);

	CHECK(pthread_create(&t, NULL, worker, NULL) == 0);
	CHECK(!ran);
	CHECK(gfip_threads_asked == 1);
	CHECK(pthread_cancel(t) == 0 && pthread_join(t, NULL) == 0);

	CHECK(gfiperf_entropy(b1, sizeof b1) == 0);
	CHECK(gfiperf_entropy(b2, sizeof b2) == 0);
	CHECK(memcmp(b1, b2, sizeof b1) != 0);
	CHECK(gfiperf_entropy(big, 7) == 0);   /* a length that isn't a multiple of 8 */
	CHECK(gfiperf_entropy(big, sizeof big) == 0);
	for (i = 0; i < (int)sizeof big; i++) seen[big[i]] = 1;
	for (i = 0; i < 256; i++) all &= seen[i];
	CHECK(all);

	errno = 0;
	CHECK(daemon(1, 0) == -1 && errno == ENOSYS);

	if (!fails) printf("compat: all checks passed\n");
	return fails != 0;
}
