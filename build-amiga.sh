#!/bin/sh
# Builds iperf3, the network bandwidth tester, for GFOS (AmigaOS 3.2.3 with
# GFOpenUp) with a stove: 68040, soft float with the FPU taken when there is
# one (fpuauto), sockets through libnix's socket layer on bsdsocket.library
# (GFOpenSocket, or any stack). The test streams run in iperf's own task
# (amiga/iperf_onetask.c) in place of upstream's thread per stream.
#   STOVE=.../stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
# iperf and fpuauto are fetched at the commits iperf3.lock gives; nothing
# third-party is kept here. The PC tests (tests/host-iperf.sh: the same
# sources and settings, upstream's unit tests and real runs on the
# loopback) run first when there is a PC compiler; HOST_TESTS=0 skips them.
# NDK_INCLUDE=... adds an OS header folder after the stove's own (a stove
# that has none, such as the bare gfopengcc release with GFOpenSDK's
# OpenNDK/Include_H).
# OUT gets Parts/iperf3: a GFOpenUp part (its Part file, C:iperf3, the
# manual and licences in SYS:Docs/iperf3), installed with
#   GFOpenUpTool INSTALL Parts/iperf3
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$HERE/iperf3.lock"
. "$HERE/scripts/iperf-common.sh"
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
# outside the checkout: Cradle cleans it (git clean -ffdx) before every job
WORK="${WORK:-$HOME/.cache/gfamigaiperf3/build}"; OUT="${OUT:-$WORK/out}"; JOBS=${JOBS:-8}
TC="$STOVE/prefix"; PATH="$TC/bin:$PATH"; export PATH
mkdir -p "$WORK/deps" "$OUT"; OUT=$(cd "$OUT" && pwd)
NDK=""; [ -z "${NDK_INCLUDE:-}" ] || NDK="-idirafter $(cd "$NDK_INCLUDE" && pwd)"
t0=$(date +%s)

# --- the PC tests first: the same sources and settings ------------------------
if [ "${HOST_TESTS:-1}" != 0 ] && command -v "${HOST_CC:-cc}" > /dev/null 2>&1; then
    WORK="$WORK/host" sh "$HERE/tests/host-iperf.sh" > "$WORK/host-tests.log" 2>&1 || { grep -E "passed|FAIL|^  " "$WORK/host-tests.log" | tail -40; exit 1; }
    grep -E "all checks passed" "$WORK/host-tests.log"
fi

# --- fpuauto: libfpuauto.a and its link specs ---------------------------------
FPD="$WORK/deps/fpuauto"
if [ "$(git -C "$FPD" rev-parse HEAD 2>/dev/null)" != "$FPA_COMMIT" ]; then
    rm -rf "$FPD"; mkdir -p "$FPD"; git -C "$FPD" init -q
    git -C "$FPD" config core.sparseCheckout true
    echo "/common/fpuauto/" > "$FPD/.git/info/sparse-checkout"
    git -C "$FPD" fetch -q --depth 1 --filter=blob:none "$FPA_REPO" "$FPA_COMMIT"
    git -C "$FPD" checkout -q FETCH_HEAD
fi
FA="$FPD/common/fpuauto"; FAL="$WORK/fpuauto"
if [ ! -f "$FAL/libfpuauto.a" ] || [ "$(cat "$FAL/.rev" 2>/dev/null)" != "$FPA_COMMIT" ]; then
    rm -rf "$FAL"; mkdir -p "$FAL/obj"
    m68k-amigaos-gcc -m68040 -DFPUAUTO_COMMON -c "$FA/fpuauto.S" -o "$FAL/obj/fa_common.o"
    for n in $(sed 's/^--wrap=//' "$FA/fpuauto.wraps"); do
        m68k-amigaos-gcc -m68040 -DR_$n -c "$FA/fpuauto.S" -o "$FAL/obj/fa_$n.o"
    done
    m68k-amigaos-ar rcs "$FAL/libfpuauto.a" "$FAL"/obj/fa_*.o
    echo "$FPA_COMMIT" > "$FAL/.rev"
fi

# --- iperf at its pinned commit, patches/, version.h, the probe ---------------
SRC="$WORK/iperf"
iperf_source "$SRC"
iperf_generate "$SRC"
LINK="m68k-amigaos-gcc -noixemul -m68040 $NDK -specs=$HERE/scripts/m68k-soft-m68040.specs -specs=$FA/fpuauto.specs -L$FAL"
PROBE_CC="$LINK -I$HERE/amiga/include -I$HERE/amiga" PROBE_LIBS="-lsocket" PROBE_GFOS=1 \
    iperf_probe "$SRC" "$WORK/iperf_config.h"
cp "$WORK/iperf_config.h" "$SRC/src/iperf_config.h"
echo "probe: $(grep -c '^#define HAVE_' "$WORK/iperf_config.h") answers"

# --- compile -------------------------------------------------------------------
LATE=""
if m68k-amigaos-gcc -fno-late-combine-instructions -S -o /dev/null -x c /dev/null 2>/dev/null; then LATE="-fno-late-combine-instructions"; fi
# no loops turned into memset/memcpy calls (GCC 16), address 0 is memory
CF="-noixemul -m68040 -O2 -fno-tree-loop-distribute-patterns -fno-delete-null-pointer-checks $LATE $NDK \
 -Werror=implicit-function-declaration -Werror=int-conversion -Wno-unused-parameter -Wno-cpp \
 -I$HERE/amiga/include -I$HERE/amiga -I$SRC/src -DGFIPERF_DATE=\\\"$(date -u +%d.%m.%Y)\\\""
O="$WORK/obj"
# objects made against another iperf or patch set go
[ "$(cat "$O/.src" 2>/dev/null)" = "$(cat "$SRC/.patched")" ] || { rm -rf "$O"; mkdir -p "$O"; cat "$SRC/.patched" > "$O/.src"; }
M="$O/Makefile"
{
    echo "CF = $CF"
    A=""
    for f in $IPERF_SRCS; do
        o="$(basename "$f" .c).o"; A="$A $o"
        printf '%s: %s %s\n\tm68k-amigaos-gcc $(CF) -MMD -c $< -o $@\n' "$o" "$SRC/$f" "$WORK/iperf_config.h"
    done
    for f in iperf_onetask pthread_one gfiperf_compat gfinet amiga_version; do
        A="$A amiga_$f.o"
        printf 'amiga_%s.o: %s %s\n\tm68k-amigaos-gcc $(CF) -MMD -c $< -o $@\n' "$f" "$HERE/amiga/$f.c" "$WORK/iperf_config.h"
    done
    echo "OBJS =$A"
    echo "all: \$(OBJS)"
    echo "-include *.d"
} > "$M"
(cd "$O" && nice -n 19 make -k -j"$JOBS" all > make.log 2>&1) || { grep -E 'error' "$O/make.log" | head -40; tail -20 "$O/make.log"; exit 1; }
echo "compile: $(( $(date +%s) - t0 )) s"

# --- link: soft-float specs, then fpuauto's; libnix's socket layer ---------------
# shellcheck disable=SC2046
$LINK -o "$O/iperf3.debug" $(cd "$O" && for f in $(sed -n 's/^OBJS =//p' Makefile); do echo "$O/$f"; done) \
    -lsocket -lm > "$O/link.log" 2>&1 || { head -60 "$O/link.log"; exit 1; }

# --- the GFOpenUp part: Parts/iperf3 ----------------------------------------------
P="$OUT/Parts/iperf3"; rm -rf "$P"; mkdir -p "$P/C" "$P/Docs/iperf3"
m68k-amigaos-strip -o "$P/C/iperf3" "$O/iperf3.debug"
cp "$HERE/README.md" "$P/Docs/iperf3/ReadMe"
cp "$SRC/LICENSE" "$P/Docs/iperf3/LICENSE-iperf"
cp "$SRC/src/iperf3.1" "$P/Docs/iperf3/iperf3.1"
cp "$HERE/LICENSE" "$P/Docs/iperf3/LICENSE-port"
sh "$HERE/scripts/write-part.sh" "$P" "$IPERF_VERSION"
ls -l "$P/C/iperf3"
echo "part: $(grep -c '^copy' "$P/Part") files, $(du -sk "$P" | cut -f1) KB"
echo "fpuauto: $(m68k-amigaos-nm "$O/iperf3.debug" | grep -c ' T ___wrap_') wrapped routines"
echo "total: $(( $(date +%s) - t0 )) s"
