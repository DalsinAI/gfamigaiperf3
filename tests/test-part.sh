#!/bin/sh
# PC test for scripts/write-part.sh: the Part file GFOpenUpTool reads.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
mkdir -p "$T/C" "$T/Docs/iperf3"
: > "$T/C/iperf3"; : > "$T/Docs/iperf3/ReadMe"; : > "$T/Docs/iperf3/iperf3.1"; : > "$T/Docs/iperf3/LICENSE-iperf"
sh "$HERE/../scripts/write-part.sh" "$T" 3.22
fail=0
expect() { grep -qxF "$1" "$T/Part" || { echo "FAIL: no line: $1"; fail=1; }; }
expect 'part iperf3 3.22'
expect 'needs 68040'
expect 'copy "C/iperf3" "SYS:C/iperf3"'
expect 'copy "Docs/iperf3/ReadMe" "SYS:Docs/iperf3/ReadMe"'
expect 'copy "Docs/iperf3/iperf3.1" "SYS:Docs/iperf3/iperf3.1"'
expect 'copy "Docs/iperf3/LICENSE-iperf" "SYS:Docs/iperf3/LICENSE-iperf"'
[ "$(head -1 "$T/Part")" = 'part iperf3 3.22' ] || { echo "FAIL: first line"; fail=1; }
[ "$(grep -c '^copy' "$T/Part")" = 4 ] || { echo "FAIL: file count (the Part file itself must not be listed)"; fail=1; }
sh "$HERE/../scripts/write-part.sh" "$T" 3.22   # a second run: the same file
[ "$(grep -c '^copy' "$T/Part")" = 4 ] || { echo "FAIL: second run lists the Part file"; fail=1; }
[ $fail = 0 ] && echo "Part file: all checks passed"
exit $fail
