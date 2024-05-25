#ifndef ENDIAN_H
#define ENDIAN_H
#include <memory.h>
#include "int.h"

// If you're unfamiliar with endian-ness, it's the order bytes are stored for
// values stored with more than 1 byte. In big endian, the largest hexidecimal
// place comes first. In little endian, it comes last (the bytes are reversed).
// The number 1738 is 06 CA in big endian, and CA 06 in little endian. Notice
// only the order of the bytes are reversed, but the order of the hex
// characters in each byte stays the same.
//
// Faster computers switched to little endian because it made addition,
// subtraction, etc. faster. On older machines this wasn't as big of a deal, so
// they used big endian. Since the Xbox 360 was big endian and most modern PCs
// are little endian, we need to reverse the byte order of every number stored
// using more than 1 byte before we use it.

// Byte-swap any value to the opposite endian.
#define ENDIAN_FLIP(T, val)                         \
do {                                                \
    union {                                         \
       T portable;                                  \
       u8 bytes[sizeof(T)];                         \
    }_foreign;                                      \
                                                    \
    for (size_t _i = 0; _i < sizeof(T); _i++) {     \
        size_t shift = (sizeof(T) - 1 - _i) * 8;    \
        _foreign.bytes[_i] = (val >> shift) & 0xFF; \
    }                                               \
    val = _foreign.portable;                        \
} while(0)

// Forces a float to be interpreted as a u32, so the compiler won't complain
// about bit-shifts on a floating-point type.
#define ENDIAN_FLIP_FLOAT(val) ENDIAN_FLIP(u32, *((u32*)&val))

#define ENDIAN_FLIP_24(val)           \
do {                                  \
    u32 v = s24_to_s32((u8*)&(val));  \
    ENDIAN_FLIP(u32, v);              \
    memcpy(&(val), ((u8*)&v) + 1, 3); \
} while(0)

#endif // ENDIAN_H
