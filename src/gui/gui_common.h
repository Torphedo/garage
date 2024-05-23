#ifndef GL_TYPES_H
#define GL_TYPES_H
#include <common/int.h>
#include <common/vector.h>
#include <vehicle.h>
#include "input.h"

// Making this the same type as GLuint removes lots of conversion linter warnings
typedef unsigned int gl_obj;

typedef struct {
    vec3 position;
    vec4 color;
}vertex;

typedef enum {
    MODE_MOVCAM,
    MODE_EDIT,
    MODE_MENU,
    MODE_ENUM_MAX,
}editor_mode;

typedef struct {
    vehicle* v;
    double delta_time; // Measured in seconds
    vec3s16 sel_box; // Selection box position
    input_internal prev_input;
    bool cam_allow_vertical;
    editor_mode mode;
}gui_state;

// Standard interface to facilitate multiple renderers per frame. For example, a
// UI renderer would render on top of the scene renderer and have separate code.
// This also allows multiple copies of a renderer at once.
typedef struct {
    // For initial setup, like compiling shaders or loading assets. Pass the
    // returned pointer to render() and destroy(), they use it to store data.
    void* (*init)(gui_state* gui);

    // For taking over the rendering pipeline (binding shaders, etc.), then
    // rendering on top and updating internal state (checking for input, etc.)
    void (*render)(void* ctx);

    // Free all buffers, unload all shaders, etc.
    void (*destroy)(void* ctx);
}renderer;

#endif // GL_TYPES_H
