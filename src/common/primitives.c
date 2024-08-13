#include "primitives.h"

const vertex quad_vertices[] = {
    {
        .position = {QUAD_SIZE, -1.5f, QUAD_SIZE},
        .color = {1.0f, 1.0f, 1.0f, 1.0f}
    },
    {
        .position = {QUAD_SIZE, -1.5f, -QUAD_SIZE},
        .color = {1.0f, 1.0f, 1.0f, 1.0f}
    },
    {
        .position = {-QUAD_SIZE, -1.5f, -QUAD_SIZE},
        .color = {1.0f, 1.0f, 1.0f, 1.0f}
    },
    {
        .position = {-QUAD_SIZE,  -1.5f, QUAD_SIZE},
        .color = {1.0f, 1.0f, 1.0f, 1.0f}
    }
};

model quad = {
    .vert_count = ARRAY_SIZE(quad_vertices),
    .idx_count = ARRAY_SIZE(quad_indices),
    .vertices = quad_vertices,
    .indices = quad_indices,
};

#define CUBE_COLOR {1.0f, 1.0f, 1.0f, 1.0f}
const vertex cube_vertices[] = {
    {
        .position = {CUBE_SIZE, CUBE_SIZE, CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {-CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {-CUBE_SIZE,  CUBE_SIZE, CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {-CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE},
        .color = CUBE_COLOR
    }
};

const u16 cube_indices[] = {
    0, 2, 1,
    0, 3, 2,
    4, 5, 6,
    4, 6, 7,

    0, 1, 5,
    0, 5, 4,
    2, 3, 7,
    2, 7, 6,

    2, 5, 1,
    2, 6, 5,
    0, 7, 3,
    0, 4, 7,
};

model cube = {
    .vert_count = ARRAY_SIZE(cube_vertices),
    .idx_count = ARRAY_SIZE(cube_indices),
    .vertices = cube_vertices,
    .indices = cube_indices,
};
