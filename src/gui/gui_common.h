#ifndef GUI_COMMON_H
#define GUI_COMMON_H
#include <common/int.h>
#include <common/vector.h>
#include <vehicle.h>
#include "input.h"

// Making this the same type as GLuint removes lots of conversion linter warnings
typedef unsigned int gl_obj;

typedef enum {
    MODE_MOVCAM, // Selection box locked, camera unlocked (freecam)
    MODE_EDIT, // Camera locked, WASD moves the selection box
    MODE_MENU, // Camera and selection box locked, some menu is enabled
    MODE_ENUM_MAX, // Increment mode and modulo by this to cycle through modes
}editor_mode;

typedef enum {
    SEL_NONE,   // User is still selecting parts to move
    SEL_ACTIVE, // User is moving parts around, and the selection isn't overlapping anything.
    SEL_BAD,    // User is moving parts around, but placing it down now would cause an overlap.
}selection_state;

// Current state of the vehicle editor & GUI in general
typedef struct {
    // Vehicle/part data
    vehicle* v;
    // Bitmask for whether a space is occupied by a part, at 1 bit per cell.
    vehicle_bitmask* vehiclemask;

    // Editor state data
    vec3s16 sel_box; // Selection box position
    bool cam_allow_vertical; // Allow vertical movement with WASD in freecam?
    editor_mode mode;
    selection_state sel_mode;

    // Extra state that doesn't affect what the user sees
    double delta_time; // Measured in seconds
    input_internal prev_input; // Input from last frame
    bool vsync;

    // Rendering state that all renderers can re-use
    gl_obj vcolor_shader; // Shader for drawing objects with vertex colors
    // Uniforms for the shader
    gl_obj u_pvm; // PVM matrix uniform
    gl_obj u_paint; // Vertex color multiplier
}gui_state;

// Update the GUI state according to new user input.
bool gui_update_with_input(gui_state* gui, GLFWwindow* window);

// Compile the basic common shader, upload buffers for primitives, setup uniforms
bool gui_init(gui_state* gui);

// Delete resources created in gui_init().
void gui_teardown(gui_state* gui);

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

typedef enum {
    STATE_NONE,
    STATE_INIT_FAIL,
    STATE_OK,
    STATE_DESTROYED,
}renderer_state;

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

#endif // GUI_COMMON_H 
