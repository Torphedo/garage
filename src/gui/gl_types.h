#ifndef GL_TYPES_H
#define GL_TYPES_H
#include <common/int.h>
#include <common/vector.h>

// Making this the same type as GLuint removes lots of conversion linter warnings
typedef unsigned int gl_obj;

typedef struct {
    vec3f position;
    vec2f tex_coord;
}vertex;


#endif // GL_TYPES_H
