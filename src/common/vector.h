#ifndef VECTOR_H
#define VECTOR_H
#include "int.h"
#include <cglm/cglm.h>
#include <cglm/struct.h>

void vec3_byteswap(vec3* v);

typedef struct {
    u8 x;
    u8 y; // Height
    u8 z;
}vec3u8;
vec3s vec3_from_vec3u8(vec3u8 vu8, float scale);

// 8-bit RGBA color
typedef struct {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
}rgba8;
vec4s vec4_from_rgba8(rgba8 c);

#endif // VECTOR_H
