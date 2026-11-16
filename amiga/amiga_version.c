/* iperf3's $VER string and stack: the Shell's 4 KB is too little for its
 * option parsing and JSON output, so libnix's start-up gives it 64 KB.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "iperf_config.h"
#ifndef GFIPERF_DATE
#define GFIPERF_DATE "16.11.2026"
#endif
const char gfiperf_ver[] __attribute__((used)) = "$VER: iperf3 " PACKAGE_VERSION " (" GFIPERF_DATE ") GFOS";
unsigned long __stack = 65536;
