#ifndef MODEL_H
#define MODEL_H
#include "common/vector.h"

typedef struct {
    vec3 position;
    vec4 color;
}vertex;

typedef struct {
    const vertex* vertices;
    const u16* indices;
    u16 vert_count;
    u16 idx_count;
    gl_obj vao;
    gl_obj vbuf;
    gl_obj ibuf;
}model;

void model_upload(model* m);
u32 model_size(model m);

// Load an OBJ file. Assumes vertex colors are stored as RGB values on each vertex.
model obj_load(const char* path);

#endif // MODEL_H
