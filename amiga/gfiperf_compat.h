/* What iperf needs that libnix doesn't have, for GFOS (amiga/gfiperf_compat.c).
 * Taken into every file by build-amiga.sh (-include).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GFIPERF_COMPAT_H
#define GFIPERF_COMPAT_H
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include "gfinet.h"

/* the stream payload and the test's cookie (patches/0003) */
int gfiperf_entropy(void *out, size_t outsize);

#ifdef GFIPERF_NEED_GETLINE
/* iperf_util.c has one; no header declares it */
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif
#ifdef GFIPERF_NEED_UNAME
struct utsname;
int uname(struct utsname *u);
#endif
#ifdef GFIPERF_NEED_STRSIGNAL
char *strsignal(int sig);
#endif
#ifdef GFIPERF_NEED_TERMIOS_DECL
/* in libnix, but no header declares them */
struct termios;
int tcgetattr(int fd, struct termios *t);
int tcsetattr(int fd, int how, const struct termios *t);
#endif
/* -D: there is nothing to detach from, so iperf says it can't (HAVE_DAEMON) */
int daemon(int nochdir, int noclose);
#endif
