#include <memory.h>
#include <stdio.h>
#include <stddef.h>

#include "int.h"

// Get a value from a bitmask like it's an array
u8 mask_get(u8* mask, u8 idx) {
    const u8* addr = mask + (idx / 8);

    // Adjust for index not divisible by 8, and return only 1 bit.
    return (*addr >> (idx % 8)) & 1;
}

// Set a value in a bitmask like it's an array
void mask_set(u8* mask, u8 idx, u8 val) {
    u8* addr = mask + (idx / 8);
    val &= 1; // Keep only 1 bit
    val <<= (idx % 8); // Adjust value for sub-byte index

    *addr |= val;
}


s32 s24_to_s32(u8* addr) {
    /*
    typedef union {
        struct {
            s32 val : 24;
        }s24;
        u8 bytes[3];
    }s24_conv;
    s24_conv* c = (s24_conv*)addr;

    // We only access the first 3 bytes via bitfield, so this is probably fine?
    return c->s24.val;
    */

    s32 val = 0;
    memcpy(&val, addr, 3);
    return val;
}

u32 exponent(u32 val, u8 pow) {
    u32 out = val;
    for (u8 i = 1; i < pow; i++) {
        out *= val;
    }
    return out;
}

void print_c16s(const c16* str) {
    // On platforms where wchar_t is 4 bytes (Linux), we have to print the
    // 2-byte characters one at a time, with the other 2 bytes set to 0.
    while (*str != 0) {
        wchar_t c = *str;
        printf("%lc", c);
        str++;
    }
}

