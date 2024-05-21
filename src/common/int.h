#ifndef INT_H
#define INT_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// UTF-16 for reading wchar_t strings built on Windows
typedef u16 c16;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

enum {
    INT24_MAX = 0xFFFFFF
};

s32 s24_to_s32(u8* addr);
u32 exponent(u32 val, u8 pow);

// Print a 16-bit string as if it were 8-bit (portably, unlike %ws)
void print_c16s(const c16* str);


#endif // INT_H
