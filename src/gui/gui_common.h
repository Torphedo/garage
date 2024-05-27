#ifndef GL_TYPES_H
#define GL_TYPES_H
#include <common/int.h>
#include <common/vector.h>
#include <vehicle.h>
#include "input.h"

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
    vehicle* v; // All vehicle data
    double delta_time; // Measured in seconds
    vec3s16 sel_box; // Selection box position
    input_internal prev_input; // Input from last frame
    bool cam_allow_vertical; // Whether to allow vertical movement with WASD in freecam
    editor_mode mode;

    selection_state sel_mode;
    // TODO: Maybe we could abuse a padding field in the part to store whether
    // it's selected, instead of keeping our own separate list.
    u16 sel_idx; // Currently selected part. Will have to refactor for multi-select later.
}gui_state;

// Update the GUI state according to new user input.
bool gui_update_with_input(gui_state* gui, GLFWwindow* window);

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

// Making this the same type as GLuint removes lots of conversion linter warnings
typedef unsigned int gl_obj;

typedef struct {
    vec3 position;
    vec4 color;
}vertex;

#endif // GL_TYPES_H
