/* POSIX <endian.h> compatibility shim for MSVC
 * Windows / x86-64 is always little-endian, so LE conversions are no-ops
 * and BE conversions use MSVC byte-swap intrinsics.
 */
#pragma once
#ifndef _COMPAT_ENDIAN_H
#define _COMPAT_ENDIAN_H

#ifdef _MSC_VER
#include <stdlib.h> /* _byteswap_* intrinsics */
#include <stdint.h>

/* Little-endian → host (no-op on x86/x64) */
#ifndef le16toh
#define le16toh(x) ((uint16_t)(x))
#endif
#ifndef le32toh
#define le32toh(x) ((uint32_t)(x))
#endif
#ifndef le64toh
#define le64toh(x) ((uint64_t)(x))
#endif

/* Host → little-endian (no-op on x86/x64) */
#ifndef htole16
#define htole16(x) ((uint16_t)(x))
#endif
#ifndef htole32
#define htole32(x) ((uint32_t)(x))
#endif
#ifndef htole64
#define htole64(x) ((uint64_t)(x))
#endif

/* Big-endian → host (byte-swap) */
#ifndef be16toh
#define be16toh(x) _byteswap_ushort((uint16_t)(x))
#endif
#ifndef be32toh
#define be32toh(x) _byteswap_ulong((uint32_t)(x))
#endif
#ifndef be64toh
#define be64toh(x) _byteswap_uint64((uint64_t)(x))
#endif

/* Host → big-endian (byte-swap) */
#ifndef htobe16
#define htobe16(x) _byteswap_ushort((uint16_t)(x))
#endif
#ifndef htobe32
#define htobe32(x) _byteswap_ulong((uint32_t)(x))
#endif
#ifndef htobe64
#define htobe64(x) _byteswap_uint64((uint64_t)(x))
#endif

#endif /* _MSC_VER */
#endif /* _COMPAT_ENDIAN_H */
