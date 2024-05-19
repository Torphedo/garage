#include "vector.h"
#include "endian.h"

vec3f vec3f_byteswap(vec3f v) {
    // The byte swap is done in-place, so we have to make a new vector first.
    vec3f out = {v.x, v.y, v.z};
    ENDIAN_FLIP_FLOAT(out.x);
    ENDIAN_FLIP_FLOAT(out.y);
    ENDIAN_FLIP_FLOAT(out.z);

    return out;
}
