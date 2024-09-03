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
#include "render_text.h"

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

    // Render this many part search results at a time
    PARTSEARCH_MENUSIZE = 10,
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

    // UI state
    text_state part_name;
    char partname_buf[32]; // Backing text buffer, enough for longest part name
    text_state editing_mode;
    text_state camera_mode_text;
    text_state textbox;
    text_state partsearch_results[PARTSEARCH_MENUSIZE];
    // Skip past this many matching entries before we start rendering (resets when
    // target string changes, used to fake scrolling)
    u32 partsearch_startoffset;
    u8 partsearch_selected_item; // Index of selected search result
    u32 partsearch_filled_slots; // Number of non-empty search result slots


    // Extra state that doesn't affect what the user sees
    double delta_time; // Measured in seconds
    input_internal prev_input; // Input from last frame
    bool vsync;
    bool init_result; // Only used during init to communicate failure
    // TODO: Can't we just pass the window pointer to the UI init function?
    GLFWwindow* window; // Used to register keyboard callbacks

    // Rendering state that everyone can re-use
    gl_obj vcolor_shader; // Shader for drawing objects with vertex colors
    // Uniforms for the shader
    gl_obj u_pvm; // PVM matrix uniform
    gl_obj u_paint; // Vertex color multiplier
}editor_state;

// TODO: Move input code out of common/ and move these functions into there
// This collection of functions lets us check for a user intent like "forward",
// instead of separately checking for the W key, gamepad stick thresholds,
// arrow keys, and the dpad up key.

// "Rising edge" means the condition is only true on the first frame the
// relevant button is pressed. (Thinking of "pressed" as a electrical signal
// set high, it triggers only on the edge where the signal rises). Honestly,
// I found out about this terminology from Minecraft redstone circuits :)

// These are all written to roughly reflect a gamepad, check the implementation
// to see what the keyboard mappings are. I didn't want to document those b/c
// the comments will quickly be outdated if the keys change

// A "confirm" action (like A button)
bool confirm_rising_edge(editor_state editor);

// A "cancel" action (like B button)
bool cancel_rising_edge(editor_state editor);

// A "pause" action (like start button)
bool pause_rising_edge(editor_state editor);

// An "up" action (like dpad)
bool up_rising_edge(editor_state editor);

// A "down" action (like dpad)
bool down_rising_edge(editor_state editor);

// A "left" action (like dpad)
bool left_rising_edge(editor_state editor);

// A "right" action (like dpad)
bool right_rising_edge(editor_state editor);

// This has less of a gamepad equivalent, but is like the space key
bool vertical_up_rising_edge(editor_state editor);

// This has less of a gamepad equivalent, but is like a shift or crouch key
bool vertical_down_rising_edge(editor_state editor);

// Unit direction vector of movement on the X axis ("A/D" key or LS X axis)
// Returns -1, 0, or 1.
s8 move_x_rising_edge(editor_state editor);

// Unit direction vector of movement on the Y axis ("W/S" key or LS Y axis)
// Returns -1, 0, or 1.
s8 move_y_rising_edge(editor_state editor);


void render_vehicle_bitmask(editor_state* editor, vehicle_bitmask* mask);

// Update our state according to new user input.
bool editor_update_with_input(editor_state* editor, GLFWwindow* window);

// Compile the common vertex color-based shader, upload buffers for primitives,
// setup uniforms & camera, load vehicle data
editor_state editor_init(const char* vehicle_path, GLFWwindow* window);

// Delete resources created in editor_init().
void editor_teardown(editor_state* editor);

#endif // EDITOR_H
