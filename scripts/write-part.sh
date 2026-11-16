#!/bin/sh
# Writes the Part file GFOpenUpTool INSTALL reads (gfopenup src/openuptool.c)
# for the part drawer the build made: C/iperf3 to C:iperf3 and Docs/iperf3/
# to SYS:Docs/iperf3/. iperf3 needs bsdsocket.library, which GFOS has.
#   write-part.sh <part drawer> <version>
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
P=$1; V=$2
{
    echo "part iperf3 $V"
    echo "; iperf3: network bandwidth tests, TCP and UDP, client and server (C:iperf3)"
    echo "needs 68040"
    (cd "$P" && find C Docs -type f | LC_ALL=C sort) | while read -r f; do
        echo "copy \"$f\" \"SYS:$f\""
    done
} > "$P/Part"
