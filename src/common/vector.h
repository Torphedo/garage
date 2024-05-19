#ifndef VECTOR_H
#define VECTOR_H
#include "int.h"

typedef struct {
    float x;
    float y;
    float z;
}vec3f;
vec3f vec3f_byteswap(vec3f v);

typedef struct {
    u8 x;
    u8 y; // Height
    u8 z;
}vec3u8;

// 8-bit RGBA color
typedef struct {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
}rgba8;

#endif // VECTOR_H
