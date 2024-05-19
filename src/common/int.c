#include <memory.h>

#include "int.h"

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
