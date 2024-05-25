#include "vector.h"
#include "endian.h"

void vec3_byteswap(vec3* v) {
    // The byte swap is done in-place
    for (u8 i = 0; i < 3; i++) {
        ENDIAN_FLIP_FLOAT(((float*)v)[i]);
    }
}

vec3s vec3_from_vec3u8(vec3u8 vu8, float scale) {
    return (vec3s) {
        .x = vu8.x * scale,
        .y = vu8.y * scale,
        .z = vu8.z * scale,
    };
}

bool vec3u8_eq(vec3u8 a, vec3u8 b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

bool vec3u8_eq_vec3s16(vec3u8 a, vec3s16 b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

vec4s vec4_from_rgba8(rgba8 c) {
    return (vec4s) {
        (float)c.r / 255,
        (float)c.g / 255,
        (float)c.b / 255,
        (float)c.a / 255,
    };
}

