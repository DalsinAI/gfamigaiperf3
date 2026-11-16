/* What iperf needs that libnix doesn't have, for GFOS.
 *   gfiperf_entropy: bytes for the stream payload and the test's cookie
 *     (iperf reads /dev/urandom elsewhere). Not for keys: iperf's cookie
 *     only tells one test from another. splitmix64 over the time, the
 *     CPU clock, a stack address and a count, so two runs (or two streams)
 *     never share a sequence.
 *   uname and strsignal (when the probe finds the C library has none),
 *   daemon. (getline is iperf's own, iperf_util.c; the header here only
 *   declares it.)
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#if defined(__has_include)
#if __has_include("iperf_config.h")
#include "iperf_config.h"   /* the probe's GFIPERF_NEED_ answers */
#endif
#endif
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "gfiperf_compat.h"
#ifdef GFIPERF_NEED_UNAME
#include <sys/utsname.h>
#include <unistd.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#endif

static uint64_t mix(uint64_t *s)
{
    uint64_t z = (*s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

int gfiperf_entropy(void *out, size_t outsize)
{
    static uint64_t state;
    static unsigned long calls;
    unsigned char *p = out;
    struct timeval tv;
    volatile int here = 0;

    gettimeofday(&tv, NULL);
    state ^= ((uint64_t)tv.tv_sec << 20) ^ (uint64_t)tv.tv_usec;
    state ^= (uint64_t)clock() << 32;
    state ^= (uint64_t)(uintptr_t)&here;
    state ^= (uint64_t)++calls << 48;
    while (outsize) {
        uint64_t v = mix(&state);
        size_t k = outsize < 8 ? outsize : 8;
        memcpy(p, &v, k);
        p += k;
        outsize -= k;
    }
    return 0;
}


#ifdef GFIPERF_NEED_UNAME
/* "AmigaOS <host> 47.111 exec 47.111 m68k (68040)": what iperf -V and its
   JSON "system_info" print */
int uname(struct utsname *u)
{
    struct ExecBase *eb = *(struct ExecBase **)4;
    UWORD af = eb->AttnFlags;
    const char *cpu = (af & 0x80) ? "68060" : (af & AFF_68040) ? "68040" :
                      (af & AFF_68030) ? "68030" : (af & AFF_68020) ? "68020" : "680x0";

    memset(u, 0, sizeof(*u));
    strncpy(u->sysname, "AmigaOS", sizeof(u->sysname) - 1);
    if (gethostname(u->nodename, sizeof(u->nodename) - 1) != 0)
        strncpy(u->nodename, "amiga", sizeof(u->nodename) - 1);
    snprintf(u->release, sizeof(u->release), "%u.%u",
             (unsigned)eb->LibNode.lib_Version, (unsigned)eb->SoftVer);
    snprintf(u->version, sizeof(u->version), "exec %u.%u",
             (unsigned)eb->LibNode.lib_Version, (unsigned)eb->LibNode.lib_Revision);
    snprintf(u->machine, sizeof(u->machine), "m68k (%s)", cpu);
    return 0;
}
#endif

#ifdef GFIPERF_NEED_STRSIGNAL
#include <signal.h>
char *strsignal(int sig)
{
    static char buf[24];
    switch (sig) {
    case SIGINT: return "Interrupt";
    case SIGTERM: return "Terminated";
#ifdef SIGHUP
    case SIGHUP: return "Hangup";
#endif
#ifdef SIGPIPE
    case SIGPIPE: return "Broken pipe";
#endif
    default: snprintf(buf, sizeof buf, "Signal %d", sig); return buf;
    }
}
#endif

int daemon(int nochdir, int noclose)
{
    (void)nochdir;
    (void)noclose;
    errno = ENOSYS;
    return -1;
}
