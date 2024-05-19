#ifndef ENDIAN_H
#define ENDIAN_H
#include <memory.h>
#include "int.h"

// Byte-swap any value to the opposite endian.
#define ENDIAN_FLIP(T, val)                        \
do {                                               \
    union {                                        \
       T portable;                                 \
       u8 bytes[sizeof(T)];                        \
    }foreign;                                      \
                                                   \
    for (size_t _i = 0; _i < sizeof(T); _i++) {    \
        size_t shift = (sizeof(T) - 1 - _i) * 8;   \
        foreign.bytes[_i] = (val >> shift) & 0xFF; \
    }                                              \
    val = foreign.portable;                        \
} while(0)

// Forces a float to be interpreted as a u32
#define ENDIAN_FLIP_FLOAT(val) ENDIAN_FLIP(u32, *((u32*)&val))

#define ENDIAN_FLIP_24(val)          \
do {                                 \
    u32 v = s24_to_s32((u8*)&(val)); \
    ENDIAN_FLIP(u32, v);             \
    memcpy(&(val), ((u8*)&v) + 1, 3);           \
} while(0)

#endif // ENDIAN_H
