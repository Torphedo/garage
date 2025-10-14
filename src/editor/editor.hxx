#pragma once
#include <cassert>

#include <common/int.h>
#include <common/list.h>

#include <layer.hxx>
extern "C" {
#include <vector.h>
#include <model.h>

#include <vehicle.h>
#include <common/gl/input.h>
}

#include "camera.hxx"
#include "render_text.hxx"

typedef enum : u8 {
    MODE_MOVCAM, // Selection box locked, camera unlocked (freecam)
    MODE_EDIT, // Camera locked, WASD moves the selection box
    MODE_MENU, // Camera and selection box locked, some menu is enabled
    MODE_ENUM_MAX, // Increment mode and modulo by this to cycle through modes
}editor_mode;

typedef enum : u8 {
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
// If I could go back and undo 1 architectural decision, I'd probably remove
// this and just loop through each part and every cell it occupies. It's
// probably not a major performance hit, and would keep the code much simpler.

// Current state of the vehicle editor & GUI in general.
struct editor_state : gui_layer {
    // Vehicle/part data
    vehicle_header v = {};
    list selected_parts = {};
    list unselected_parts = {};
    camera cam;
    // Bitmask for whether a space is occupied by a part, at 1 bit per cell.
    vehicle_bitmask* vacancy_mask = nullptr;
    // Bitmask for whether a cell is selected
    vehicle_bitmask* selected_mask = nullptr;

    // Editor state data
    vec3s16 sel_box = {}; // Selection box position
    editor_mode mode = MODE_MOVCAM;
    selection_state sel_mode = SEL_NONE;

    // Extra state that doesn't affect what the user sees
    double one_frame_ago = 0.0;
    double two_frames_ago = 0.0;
    double delta_time = 0.0; // Measured in seconds
    input_internal prev_input = {}; // Input from last frame
    bool vsync = true;
    bool init_result = false; // Only used during init to communicate failure
    // TODO: Can't we just pass the window pointer to the UI init function?

    // Rendering state that everyone can re-use
    gl_obj vcolor_shader = 0; // Shader for drawing objects with vertex colors
    // Uniforms for the shader
    gl_obj u_pvm = 0; // PVM matrix uniform
    gl_obj u_paint = 0; // Vertex color multiplier

    // This collection of functions lets us check for a user intent like "forward",
    // instead of separately checking for the W key, gamepad stick thresholds,
    // arrow keys, and the dpad up key across the whole codebase.

    // "Rising edge" means the condition is only true on the first frame the
    // relevant button is pressed. (Thinking of "pressed" as a electrical signal
    // set high, it triggers only on the edge where the signal rises). Honestly,
    // I found out about this terminology from Minecraft redstone circuits :)

    // These are all written to roughly reflect a gamepad, check the implementation
    // to see what the keyboard mappings are. I didn't want to document those b/c
    // the comments will quickly be outdated if the keys change

    // A "confirm" action (like A button)
    bool confirm_rising_edge() const noexcept;

    // A "cancel" action (like B button)
    bool cancel_rising_edge() const noexcept;

    // A "pause" action (like start button)
    bool pause_rising_edge() const noexcept;

    // An "up" action (like dpad)
    bool up_rising_edge() const noexcept;

    // A "down" action (like dpad)
    bool down_rising_edge() const noexcept;

    // A "left" action (like dpad)
    bool left_rising_edge() const noexcept;

    // A "right" action (like dpad)
    bool right_rising_edge() const noexcept;

    // This has less of a gamepad equivalent, but is like the space key
    bool vertical_up_rising_edge() const noexcept;

    // This has less of a gamepad equivalent, but is like a shift or crouch key
    bool vertical_down_rising_edge() const noexcept;

    // Unit direction vector of movement on the X axis ("A/D" key or LS X axis)
    // Returns -1, 0, or 1.
    s8 move_x_rising_edge() const noexcept;

    // Unit direction vector of movement on the Y axis ("W/S" key or LS Y axis)
    // Returns -1, 0, or 1.
    s8 move_y_rising_edge() const noexcept;

    void update_edit_mode() noexcept;

    void render_vehicle_bitmask(vehicle_bitmask* mask) const noexcept;

    explicit editor_state(const char* vehicle_path) noexcept;

    // Compile the common vertex color-based shader, upload buffers for primitives,
    // setup uniforms & camera, load vehicle data
    void init(GLFWwindow* window) noexcept override;

    // Update our state according to new user input.
    void update(GLFWwindow* window) noexcept override;

    // Delete resources created in init().
    void destroy() noexcept override;
};