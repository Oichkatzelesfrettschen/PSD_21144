#ifndef _MACHINE_ENDIAN_H_
#define _MACHINE_ENDIAN_H_

// This is a dummy file for standalone compilation to satisfy #include <machine/endian.h>
// from sys/types.h.

// Define byte order macros as little-endian for x86-64 (common for testing).
#define _LITTLE_ENDIAN  1234
#define _BIG_ENDIAN     4321
#define _PDP_ENDIAN     3412

#define BYTE_ORDER _LITTLE_ENDIAN

// Basic byte swap functions if anything in types.h or other headers needs them.
// Usually, full kernel code would have optimized assembly for these.
#if __GNUC__ >= 4
static __inline __uint16_t
__bswap16(__uint16_t _x)
{
  return (__uint16_t)__builtin_bswap16(_x);
}

static __inline __uint32_t
__bswap32(__uint32_t _x)
{
  return (__uint32_t)__builtin_bswap32(_x);
}

static __inline __uint64_t
__bswap64(__uint64_t _x)
{
  return (__uint64_t)__builtin_bswap64(_x);
}
#else
// Fallback for non-GCC or older GCC
static __inline __uint16_t
__bswap16(__uint16_t _x) {
    return ((_x >> 8) & 0xff) | ((_x << 8) & 0xff00);
}
static __inline __uint32_t
__bswap32(__uint32_t _x) {
    return ((_x >> 24) & 0xff) | ((_x >> 8) & 0xff00) |
           ((_x << 8) & 0xff0000) | ((_x << 24) & 0xff000000);
}
static __inline __uint64_t
__bswap64(__uint64_t _x) {
    return ((__uint64_t)__bswap32(_x & 0xffffffff) << 32) | __bswap32(_x >> 32);
}
#endif // __GNUC__


#define ntohs(x) __bswap16(x)
#define htons(x) __bswap16(x)
#define ntohl(x) __bswap32(x)
#define htonl(x) __bswap32(x)
#define ntohll(x) __bswap64(x)
#define htonll(x) __bswap64(x)

#endif /* _MACHINE_ENDIAN_H_ */
