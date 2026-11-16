# GFAmigaIPerf3

iperf3 3.22, the network bandwidth tester, for GFOS (AmigaOS 3.2.3 with
GFOpenUp) on a 68040 or 68060, installed as a GFOpenUp part: `C:iperf3`.
Client and server, TCP and UDP, and it speaks to any other iperf3.

    iperf3 -s                       ; on the Amiga: a server
    iperf3 -c 192.168.1.20 -t 10    ; or a client, against a PC's iperf3 -s
    iperf3 -c 192.168.1.20 -R       ; the other way: the PC sends
    iperf3 -c 192.168.1.20 -u -b 5M ; UDP at 5 Mbit/s: loss and jitter

Status: built for the PC from the same sources and settings and passing
iperf's own unit tests and real runs on the loopback, against itself and
against a stock iperf3 both ways (see Tests); the Amiga build is made by
the stove, not yet run in a lab.

## What you need

- GFOS: AmigaOS 3.2 with GFOpenUp, a 68040 or 68060. Soft float: the FPU
  is used when there is one (fpuauto), so an LC040 runs it too.
- A TCP/IP stack: `bsdsocket.library` (GFOpenSocket on GFOS, or any other).

Install with `GFOpenUpTool INSTALL Parts/iperf3`. The manual is
`SYS:Docs/iperf3/iperf3.1` (the manual page; https://iperf.fr has the same).

## On AmigaOS

- IPv4. An IPv6 address answers "Address family not supported", and the
  server says once that it is taking IPv4 ("this system does not seem to
  support IPv6").
- The test streams run in iperf's own task. Upstream (3.16 and later)
  gives each stream a thread; on AmigaOS a socket belongs to the task that
  opened `bsdsocket.library`, so here the main loop waits on every stream
  with the control connection and sends or receives on the ones that are
  ready (`amiga/iperf_onetask.c`). Several streams (`-P`) share the one
  CPU, as they would anyway on one 68k.
- `--bitrate` is held by checking the rate each round, at most 1 ms
  apart, in place of the threads' sleeps.
- Not offered: `-D` (daemon: run it with `Run` instead), `-Z` (zerocopy:
  there is no `sendfile`), `--username`/`--rsa-*` (authentication needs
  OpenSSL), CPU affinity, MPTCP, SCTP, flow labels, `--fq-rate`.
- 64 KB of stack is set in the program, so the Shell's 4 KB is enough.

## Building

    STOVE=~/AmiCradle/stoves/os32-gcc16-open6 OUT=out sh build-amiga.sh

or in Cradle, stove_build with the os32-gcc16-open6 stove, target `.`.
iperf and fpuauto are fetched at the commits `iperf3.lock` pins.
`OUT` gets `Parts/iperf3` (the part). A stove without its own NDK (the bare
gfopengcc release) takes `NDK_INCLUDE=.../gfopensdk/OpenNDK/Include_H`.

Sockets are libnix's socket layer (`-lsocket`): a socket is a file
descriptor, so iperf's `read`, `write`, `close` and `select` on sockets
work as upstream wrote them.

## Tests

`make -C tests test` on a PC: the Part file, then `tests/host-iperf.sh`,
which builds iperf for the PC from the same patched sources and the same
GFOS settings (one task, `amiga/include/pthread.h`, the stream buffer in
memory, `amiga/gfinet.c`), and runs:

- `tests/compat_test.c`: the one-task mutex and thread calls, the entropy;
- `tests/gfinet_test.c`: `inet_pton`, `inet_ntop` and `getaddrinfo`
  against the C library's own (37 address forms, the passive, loopback,
  canonical-name and error cases);
- upstream's unit tests: t_units, t_timer, t_uuid, t_api;
- client against server on the loopback, each run's JSON checked by
  `tests/check_json.py`: TCP, 4 streams, reverse, both ways at once, a byte
  count, a block count, UDP at a set bit rate (within 20%, under 5% loss),
  UDP reverse, 2 UDP streams;
- when the PC has a stock iperf3: ours against it and it against ours,
  TCP and UDP, both ways.

What only a lab can show: the bsdsocket stack's speed and behaviour under
load, Ctrl-C ending a test, and the bit rate on a real 68040.

## What was changed

- `patches/0001`: the client's and server's main `select()` goes through
  `iperf_onetask_select()` when `IPERF_ONE_TASK` is defined.
- `patches/0002`: with `IPERF_NO_MMAP` the stream buffer is plain memory
  (upstream maps a temporary file).
- `patches/0003`: with `IPERF_AMIGA_ENTROPY` the payload and cookie bytes
  come from `amiga/gfiperf_compat.c` (there is no `/dev/urandom`).
- `amiga/iperf_onetask.c`: the streams served from one task;
  `amiga/include/pthread.h`, `amiga/pthread_one.c`: the thread calls for
  one task (nothing starts; the mutex counts).
- `amiga/gfinet.c`: `getaddrinfo`, `inet_pton`, `inet_ntop` of our own,
  compiled with the same headers as iperf (libnix's `getaddrinfo` was built
  against other headers, so its `struct addrinfo` might not agree).
- `amiga/gfiperf_compat.c`: entropy, `uname` and `strsignal` when the C
  library has none, `daemon` saying no.
- `amiga/include/endian.h`: big-endian byte order for `portable_endian.h`.
- `scripts/iperf-common.sh`: fetching, `version.h`, and a configure-style
  probe (each answer compiled or linked, as autoconf does) giving
  `iperf_config.h`.

iperf3 is BSD-licensed (ESnet / Lawrence Berkeley National Laboratory,
`SYS:Docs/iperf3/LICENSE-iperf`); the port's own files are MIT.

Designed, tested and delivered by The AmiCradle Team.
