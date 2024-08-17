#ifndef MODEL_H
#define MODEL_H
#include "vector.h"

// A vertex with only posiiton and color
typedef struct {
    vec3 position;
    vec4 color;
}vertex;

typedef struct {
    const void* vertices;
    const u16* indices;
    u16 vert_count;
    u16 idx_count;
    gl_obj vao;
    gl_obj vbuf;
    gl_obj ibuf;
}model;

// Upload a model to the GPU, writing the OpenGL IDs to the struct
void model_upload(model* m);
u32 model_size(const model m);

// Load OBJ data into a model struct.
// Assumes vertex colors are stored as RGB values on each vertex.
model obj_load(u8* txt);

#endif // MODEL_H
