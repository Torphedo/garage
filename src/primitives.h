#ifndef PRIMITIVES_H
#define PRIMITIVES_H
/// @file primitives.h
/// @brief Simple predefined 3D shapes (like cubes)

#include "model.h"
enum {
    QUAD_SIZE = 32,
    CUBE_SIZE = 1,
};

static const u16 quad_indices[] = {
    0, 1, 2,
    3, 0, 2,
};


extern model quad;
extern model cube;

#endif // PRIMITIVES_H
