/* <endian.h> for iperf's portable_endian.h on GFOS: the 68k is big-endian,
 * so host order is network order and the 64-bit swaps are the identity.
 * The PC test build uses the C library's own <endian.h> (this folder is
 * not on its include path for this file: tests/host-iperf.sh).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GFIPERF_ENDIAN_H
#define GFIPERF_ENDIAN_H
#include <stdint.h>
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif
#ifndef BYTE_ORDER
#define BYTE_ORDER BIG_ENDIAN
#endif
#define htobe16(x) ((uint16_t)(x))
#define htobe32(x) ((uint32_t)(x))
#define htobe64(x) ((uint64_t)(x))
#define be16toh(x) ((uint16_t)(x))
#define be32toh(x) ((uint32_t)(x))
#define be64toh(x) ((uint64_t)(x))
#endif
