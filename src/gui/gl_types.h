#ifndef GL_TYPES_H
#define GL_TYPES_H
#include <common/int.h>
#include <common/vector.h>

typedef u32 gl_obj;

typedef struct {
    vec3f position;
    vec2f tex_coord;
}vertex;


#endif // GL_TYPES_H
