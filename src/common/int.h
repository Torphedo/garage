#ifndef INT_H
#define INT_H

#include <stdint.h>

// Shorthands for the standard-size int types
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

// Useful when handling 24-bit integers not covered by the standard
enum {
    INT24_MAX = 0xFFFFFF
};

s32 s24_to_s32(u8* addr);

// Raise a number to a power (val^pow)
u32 exponent(u32 val, u8 pow);

// Print a 16-bit string as if it were 8-bit (portable, unlike %ws)
void print_c16s(const c16* str);

// Round a number up to any boundary
#define ALIGN_UP(x, bound) (((x - 1) & (bound * 0xF)) + bound)

// Return the larger of 2 values
#define MAX(a, b) ((a > b) ? a : b)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))
#endif // INT_H
