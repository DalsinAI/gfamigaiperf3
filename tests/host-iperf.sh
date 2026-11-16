#!/bin/sh
# iperf built for the PC the way build-amiga.sh builds it for GFOS: the same
# patches, the probe's iperf_config.h with the GFOS settings (streams in one
# task through amiga/iperf_onetask.c, amiga/include/pthread.h in place of
# threads, the stream buffer in memory, amiga/gfiperf_compat.c's entropy),
# then:
#   - upstream's unit tests (t_units, t_timer, t_uuid, t_api);
#   - real tests over the loopback, client against server, each checked in
#     iperf's JSON: TCP, TCP with 4 streams, reverse, both ways at once,
#     a byte count, UDP at a set bit rate (the throttle), UDP reverse;
#   - a stock iperf3 on this PC against ours, both ways, when there is one
#     (the wire protocol is upstream's).
#   sh tests/host-iperf.sh     (WORK=... keeps the build somewhere else)
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
. "$HERE/iperf3.lock"
. "$HERE/scripts/iperf-common.sh"
WORK=${WORK:-"$HERE/build-host"}; CC=${HOST_CC:-cc}
mkdir -p "$WORK"; WORK=$(cd "$WORK" && pwd)
SRC="$WORK/iperf"
iperf_source "$SRC"
iperf_generate "$SRC"
PROBE_CC="$CC" PROBE_GFOS=1 iperf_probe "$SRC" "$SRC/src/iperf_config.h"
# amiga/include holds pthread.h; endian.h there is the 68k's, so the PC
# build takes only pthread.h from it (a folder of its own)
mkdir -p "$WORK/inc"; cp "$HERE/amiga/include/pthread.h" "$WORK/inc/"
INC="-I$WORK/inc -I$HERE/amiga -I$SRC/src"
O="$WORK/obj"; rm -rf "$O"; mkdir -p "$O"
lib=""
for f in $IPERF_SRCS; do
    o="$O/$(basename "$f" .c).o"
    # shellcheck disable=SC2086
    $CC -O1 -g -w -Werror=implicit-function-declaration -Werror=int-conversion $INC -c "$SRC/$f" -o "$o"
    [ "$f" = src/main.c ] || lib="$lib $o"
done
for f in iperf_onetask pthread_one gfiperf_compat gfinet; do
    # shellcheck disable=SC2086
    $CC -O1 -g -Wall -Wextra $INC -c "$HERE/amiga/$f.c" -o "$O/$f.o"
    lib="$lib $O/$f.o"
done
# shellcheck disable=SC2086
$CC -o "$WORK/iperf3" "$O/main.o" $lib -lm
echo "built: $("$WORK/iperf3" --version | head -1)"
# no thread library: every pthread_ name is the one-task stand-in's
if nm "$WORK/iperf3" | grep -E ' U pthread_(create|join|cancel)' > /dev/null; then
    echo "FAIL: iperf3 still calls the real pthread_create"; exit 1
fi

fail=0
# the pthread stand-in and gfiperf_compat.c on their own
$CC -std=gnu99 -Wall -Wextra -I"$WORK/inc" -I"$HERE/amiga" -o "$WORK/compat_test" "$HERE/tests/compat_test.c" \
    "$HERE/amiga/pthread_one.c" "$HERE/amiga/gfiperf_compat.c" && "$WORK/compat_test" || fail=1
# amiga/gfinet.c against the C library's own
$CC -std=gnu99 -Wall -Wextra -I"$HERE/amiga" -o "$WORK/gfinet_test" "$HERE/tests/gfinet_test.c" "$HERE/amiga/gfinet.c" && "$WORK/gfinet_test" || fail=1
for t in t_units t_timer t_uuid t_api; do
    # shellcheck disable=SC2086
    $CC -O1 -g -w $INC -o "$WORK/$t" "$SRC/src/$t.c" $lib -lm
    if (cd "$SRC/src" && "$WORK/$t" > "$WORK/$t.log" 2>&1); then echo "$t: passed"
    else echo "FAIL $t:"; tail -20 "$WORK/$t.log"; fail=1; fi
done

PORT=${PORT:-$(( 20000 + $$ % 20000 ))}
SERVER_PID=""
stop_server() { [ -z "$SERVER_PID" ] || { kill "$SERVER_PID" 2>/dev/null || true; wait "$SERVER_PID" 2>/dev/null || true; SERVER_PID=""; }; }
trap stop_server EXIT
start_server() {   # start_server IPERF3
    "$1" -s -p "$PORT" --forceflush > "$WORK/server.log" 2>&1 &
    SERVER_PID=$!
    i=0; while [ $i -lt 50 ]; do
        grep -q "Server listening" "$WORK/server.log" 2>/dev/null && return 0
        sleep 0.1; i=$((i + 1))
    done
    echo "FAIL: server did not start"; cat "$WORK/server.log"; exit 1
}
# check NAME CLIENT ARGS...: one client run, its JSON checked by tests/check_json.py
check() {
    name=$1; client=$2; shift 2
    if "$client" -c 127.0.0.1 -p "$PORT" -J "$@" > "$WORK/$name.json" 2> "$WORK/$name.err" &&
       python3 "$HERE/tests/check_json.py" "$WORK/$name.json" "$@"; then
        echo "$name: passed ($*)"
    else
        echo "FAIL $name ($*):"; tail -5 "$WORK/$name.err"; head -c 600 "$WORK/$name.json"; echo; fail=1
    fi
}
start_server "$WORK/iperf3"
check tcp "$WORK/iperf3" -t 2
check tcp-4 "$WORK/iperf3" -t 2 -P 4
check reverse "$WORK/iperf3" -t 2 -R
check bidir "$WORK/iperf3" -t 2 --bidir
check bytes "$WORK/iperf3" -n 8M
check udp "$WORK/iperf3" -t 3 -u -b 20M
check udp-reverse "$WORK/iperf3" -t 2 -u -b 10M -R
check udp-2 "$WORK/iperf3" -t 2 -u -b 5M -P 2
check blocks "$WORK/iperf3" -k 200 -l 16K
stop_server
STOCK=$(command -v iperf3 || true)
if [ -n "$STOCK" ] && [ "$STOCK" != "$WORK/iperf3" ]; then
    start_server "$STOCK"
    check ours-to-stock "$WORK/iperf3" -t 2
    check ours-to-stock-udp "$WORK/iperf3" -t 2 -u -b 10M
    stop_server
    start_server "$WORK/iperf3"
    check stock-to-ours "$STOCK" -t 2
    check stock-to-ours-reverse "$STOCK" -t 2 -R
    stop_server
else
    echo "no stock iperf3 on this PC: interoperability runs skipped"
fi
[ $fail = 0 ] && echo "iperf host tests: all checks passed"
exit $fail
