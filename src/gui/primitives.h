#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "gui_common.h"
enum {
    QUAD_SIZE = 32,
    CUBE_SIZE = 1,
};

extern model quad;
extern model tex_quad; // Same as quad but with texture coords
extern model cube;

#endif // PRIMITIVES_H
