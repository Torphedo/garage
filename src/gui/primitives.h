#include "gui_common.h"

#define QUAD_SIZE (32.0f)
vec4 quad_paint = {1.0f, 0.5f, 0.2f, 1.0f};
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

const u16 quad_indices[] = {
    0, 1, 2,
    3, 0, 2,
};

model quad = {
    .vert_count = ARRAY_SIZE(quad_vertices),
    .idx_count = ARRAY_SIZE(quad_indices),
    .vertices = quad_vertices,
    .indices = quad_indices,
};

#define CUBE_SIZE (1.0f)
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
        0, 2, 3,
        4, 6, 5,
        4, 6, 7,

        0, 5, 1,
        0, 5, 4,
        2, 7, 3,
        2, 7, 6,

        2, 5, 1,
        2, 5, 6,
        0, 7, 3,
        0, 7, 4,
};

model cube = {
    .vert_count = ARRAY_SIZE(cube_vertices),
    .idx_count = ARRAY_SIZE(cube_indices),
    .vertices = cube_vertices,
    .indices = cube_indices,
};

