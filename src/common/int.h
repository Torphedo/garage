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

// Dedicated integer type for GL object IDs. Making this the same type as GLuint
// removes lots of conversion linter warnings
typedef unsigned int gl_obj;


// Useful when handling 24-bit integers not covered by the standard
enum {
    INT24_MAX = 0xFFFFFF
};

// Get a value from a bitmask like it's an array
u8 mask_get(u8* mask, u8 idx);

// Set a value in a bitmask like it's an array
void mask_set(u8* mask, u8 idx, u8 val);

s32 s24_to_s32(u8* addr);

// Raise a number to a power (val^pow)
u32 exponent(u32 val, u8 pow);

// Print a 16-bit string as if it were 8-bit (portable, unlike %ws)
void print_c16s(const c16* str);

// Round a number up to any boundary
// I remember writing this, but not where the formula came from or why it works
#define ALIGN_UP(x, bound) (((x - 1) & (bound * 0xF)) + bound)

// Return the larger of 2 values
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)
// Returns low or high bound if [val] is out of bounds, otherwise return [val]
#define CLAMP(low, val, high) ((val < low) ? low : MIN(val, high))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))
#endif // INT_H
