#ifndef VECTOR_H
#define VECTOR_H
#include "int.h"
#include <cglm/struct.h>

void vec3_byteswap(vec3* v);

// This isn't standard until C11, but a lot of compilers support it in C99.
// cglm also uses anonymous structs like this for its vector struct types.
typedef union {
    struct {
        u8 x;
        u8 y; // Height
        u8 z;
    };
    u8 raw[3];
}vec3u8;
vec3s vec3_from_vec3u8(vec3u8 vu8, float scale);
bool vec3u8_eq(vec3u8 a, vec3u8 b);

typedef union {
    struct {
        s16 x;
        s16 y; // Height
        s16 z;
    };
    s16 raw[3];
}vec3s16;
bool vec3u8_eq_vec3s16(vec3u8 a, vec3s16 b);
bool vec3s16_eq(vec3s16 a, vec3s16 b);

// 8-bit RGBA color
typedef struct {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
}rgba8;

// Useful to pass colors to a shader from a 32-bit color.
vec4s vec4_from_rgba8(rgba8 c);

#define LERP(a, b, t) ((a * (1 - t)) + (b * t))

#endif // VECTOR_H
