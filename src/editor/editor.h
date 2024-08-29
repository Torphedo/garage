#ifndef EDITOR_H
#define EDITOR_H
#include <assert.h>

#include <common/int.h>
#include <common/vector.h>
#include <common/input.h>
#include <common/model.h>
#include <common/list.h>

#include <vehicle.h>
#include "camera.h"

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

enum {
    PART_POS_SCALE = 2, // Coordinate multiplier for rendering (can add spacing in the part grid)
    VEH_MASK_BYTE_WIDTH = (VEH_MAX_DIM / 8),
};

// Used to store a compact 3D grid of parts at 1 bit per cell.
// The editor uses 2 of these, one for storing which cells are occupied and one
// for storing which cells are selected.
typedef u8 vehicle_bitmask[VEH_MAX_DIM][VEH_MAX_DIM][VEH_MASK_BYTE_WIDTH];
static_assert(sizeof(vehicle_bitmask) == 0x40000, "vehicle_bitmask size is wrong!");

// Current state of the vehicle editor & GUI in general
typedef struct {
    // Vehicle/part data
    vehicle_header v;
    list selected_parts;
    list unselected_parts;
    camera cam;
    // Bitmask for whether a space is occupied by a part, at 1 bit per cell.
    vehicle_bitmask* vacancy_mask;
    // Bitmask for whether a cell is selected
    vehicle_bitmask* selected_mask;

    // Editor state data
    vec3s16 sel_box; // Selection box position
    editor_mode mode;
    selection_state sel_mode;

    // Extra state that doesn't affect what the user sees
    double delta_time; // Measured in seconds
    input_internal prev_input; // Input from last frame
    bool vsync;
    bool init_result; // Only used during init to communicate failure
    GLFWwindow* window; // Used to register keyboard callbacks

    // Rendering state that everyone can re-use
    gl_obj vcolor_shader; // Shader for drawing objects with vertex colors
    // Uniforms for the shader
    gl_obj u_pvm; // PVM matrix uniform
    gl_obj u_paint; // Vertex color multiplier
}editor_state;

// Update our state according to new user input.
bool editor_update_with_input(editor_state* editor, GLFWwindow* window);

// Compile the basic common shader, upload buffers for primitives, setup uniforms
editor_state editor_init(const char* vehicle_path);

// Delete resources created in editor_init().
void editor_teardown(editor_state* editor);
void render_vehicle_bitmask(editor_state* editor, vehicle_bitmask* mask);

#endif // EDITOR_H
