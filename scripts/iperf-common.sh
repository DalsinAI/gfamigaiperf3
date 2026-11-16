# What build-amiga.sh (the stove) and tests/host-iperf.sh (the PC) share:
# fetching iperf, its generated files, and the probe that writes
# iperf_config.h, the answers iperf's configure would give. Sourced; needs
# HERE (the repo) and iperf3.lock's settings.
# MIT, Copyright (c) 2026 AmiCradle Team.

# libiperf and the iperf3 program, as upstream's src/Makefile.am
IPERF_SRCS="src/cjson.c src/iperf_api.c src/iperf_error.c src/iperf_auth.c
 src/iperf_client_api.c src/iperf_locale.c src/iperf_server_api.c src/iperf_tcp.c
 src/iperf_udp.c src/iperf_sctp.c src/iperf_util.c src/iperf_time.c src/iperf_pthread.c
 src/dscp.c src/net.c src/tcp_info.c src/timer.c src/units.c src/main.c"

# a repository at one commit, fetched once: fetch_at DIR URL COMMIT
fetch_at() {
    if [ "$(git -C "$1" rev-parse HEAD 2>/dev/null)" != "$3" ]; then
        rm -rf "${1:?}"; mkdir -p "$1"
        git -C "$1" init -q
        git -C "$1" fetch -q --depth 1 "$2" "$3"
        git -C "$1" checkout -q FETCH_HEAD
        [ "$(git -C "$1" rev-parse HEAD)" = "$3" ] || { echo "$1: commit mismatch"; exit 1; }
    fi
}

# iperf at IPERF_COMMIT with patches/ applied, into $1 (redone when either changes)
iperf_source() {
    PSUM=$(cat "$HERE"/patches/*.patch | sha256sum | cut -c1-16)
    if [ "$(cat "$1/.patched" 2>/dev/null)" != "$IPERF_COMMIT-$PSUM" ]; then
        rm -rf "$1"
        fetch_at "$1" "$IPERF_GIT" "$IPERF_COMMIT"
        for p in "$HERE"/patches/*.patch; do
            patch -s -d "$1" -p1 < "$p" || { echo "patch failed: $p"; exit 1; }
        done
        echo "$IPERF_COMMIT-$PSUM" > "$1/.patched"
    fi
}

# src/version.h, as configure makes it from version.h.in
iperf_generate() {
    sed "s/@PACKAGE_VERSION@/$IPERF_VERSION/" "$1/src/version.h.in" > "$1/src/version.h"
}

# iperf_config.h into $2: configure's answers, each found by compiling or
# linking a test as configure does ($PROBE_CC: the compile and link command).
# PROBE_GFOS=1: the GFOS build (one task, our compat, no mmap).
iperf_probe() {
    t=$(mktemp -d)
    hdrs='#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>'
    links() {   # links NAME: the function is in the C library and a header declares it
        printf '%s\nint main(void) { void (*volatile p)(void) = (void (*)(void))%s; return p != 0; }\n' "$hdrs" "$1" > "$t/p.c"
        # shellcheck disable=SC2086
        $PROBE_CC -o "$t/p" "$t/p.c" ${PROBE_LIBS:-} > /dev/null 2>&1
    }
    compiles() {   # compiles EXPR: the expression builds (a constant or a field)
        printf '%s\n%s\nint main(void) { return (int)(%s); }\n' "$hdrs" "${PROBE_EXTRA:-}" "$1" > "$t/p.c"
        # shellcheck disable=SC2086
        $PROBE_CC -o "$t/p" "$t/p.c" ${PROBE_LIBS:-} > /dev/null 2>&1
    }
    {
        echo "/* iperf_config.h: written by scripts/iperf-common.sh iperf_probe */"
        for h in stdint stdlib string strings stdio inttypes unistd sys/types sys/stat sys/socket poll endian linux/tcp; do
            printf '#include <%s.h>\nint main(void) { return 0; }\n' "$h" > "$t/p.c"
            # shellcheck disable=SC2086
            if $PROBE_CC ${PROBE_INC:-} -o "$t/p" "$t/p.c" > /dev/null 2>&1; then
                echo "#define HAVE_$(echo "$h" | tr '[:lower:]/' '[:upper:]_')_H 1"
            fi
        done
        echo "#define HAVE_PTHREAD 1"
        echo "#define PACKAGE \"iperf\""
        echo "#define PACKAGE_NAME \"iperf\""
        echo "#define PACKAGE_TARNAME \"iperf\""
        echo "#define PACKAGE_VERSION \"$IPERF_VERSION\""
        echo "#define PACKAGE_STRING \"iperf $IPERF_VERSION\""
        echo "#define PACKAGE_BUGREPORT \"https://github.com/DalsinAI/gfamigaiperf3/issues\""
        echo "#define PACKAGE_URL \"https://github.com/DalsinAI/gfamigaiperf3\""
        links clock_gettime && echo "#define HAVE_CLOCK_GETTIME 1"
        links getline && echo "#define HAVE_GETLINE 1"
        compiles SHUT_WR && echo "#define HAVE_SOCKET_SHUTDOWN_SHUT_WR 1"
        compiles MSG_TRUNC && echo "#define HAVE_MSG_TRUNC 1"
        compiles TCP_KEEPIDLE && echo "#define HAVE_TCP_KEEPALIVE 1"
        if [ "${PROBE_GFOS:-}" = 1 ]; then
            # one task (amiga/include/pthread.h, amiga/iperf_onetask.c): no
            # nanosleep for --bitrate, the pump checks the rate instead; no
            # stdatomic (one task, and no 64-bit atomics without libatomic)
            echo "#define IPERF_ONE_TASK 1"
            echo "#define IPERF_NO_MMAP 1"
            echo "#define IPERF_AMIGA_ENTROPY 1"
            echo "#define HAVE_DAEMON 1"
            links getline || echo "#define GFIPERF_NEED_GETLINE 1"
            links uname || echo "#define GFIPERF_NEED_UNAME 1"
            links strsignal || echo "#define GFIPERF_NEED_STRSIGNAL 1"
            compiles "sizeof(struct sockaddr_storage)" || echo "#define GFIPERF_NEED_SOCKADDR_STORAGE 1"
            compiles "sizeof(((struct sockaddr_in *)0)->sin_len)" && echo "#define GFINET_SIN_LEN 1"
            compiles "in6addr_any.s6_addr[0]" || echo "#define GFIPERF_NEED_IN6ADDR_ANY 1"
            [ -n "$(PROBE_EXTRA='#include <arpa/inet.h>' compiles 0 && echo y)" ] || echo "#define GFINET_NO_ARPA 1"
            # tcgetattr: in the library but maybe in no header
            printf '#include <termios.h>\nint main(void) { struct termios t; return tcgetattr(0, &t); }\n' > "$t/p.c"
            # shellcheck disable=SC2086
            $PROBE_CC -Werror=implicit-function-declaration -o "$t/p" "$t/p.c" ${PROBE_LIBS:-} > /dev/null 2>&1 ||
                echo "#define GFIPERF_NEED_TERMIOS_DECL 1"
            echo "#include \"gfiperf_compat.h\""
        else
            links nanosleep && echo "#define HAVE_NANOSLEEP 1"
            links daemon && echo "#define HAVE_DAEMON 1"
            printf '#include <stdatomic.h>\nint main(void) { return 0; }\n' > "$t/p.c"
            $PROBE_CC -o "$t/p" "$t/p.c" > /dev/null 2>&1 && echo "#define HAVE_STDATOMIC_H 1"
        fi
    } > "$2"
    rm -rf "$t"
}
