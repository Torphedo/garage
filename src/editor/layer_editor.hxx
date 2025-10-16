#pragma once
#include <cassert>
#include <vector>

#include <common/int.h>
#include <common/list.h>

#include <layer.hxx>
#include <action/action.hxx>

extern "C" {
#include <vector.h>
#include <model.h>

#include <vehicle.h>
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
};

// Current state of the vehicle editor & GUI in general.
struct editor_state : gui_layer {
    // Vehicle/part data
    vehicle_header v = {};
    std::vector<part_entry> selected_parts;
    std::vector<part_entry> unselected_parts;
    camera cam;
    action::map actions;

    // Editor state data
    vec3s8 sel_box = {}; // Selection box position
    editor_mode mode = MODE_MOVCAM;
    selection_state sel_mode = SEL_NONE;

    // Extra state that doesn't affect what the user sees
    double one_frame_ago = 0.0;
    double two_frames_ago = 0.0;
    double delta_time = 0.0; // Measured in seconds
    bool vsync = true;

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

    explicit editor_state(const char* vehicle_path) noexcept;

    // Compile the common vertex color-based shader, upload buffers for primitives,
    // setup uniforms & camera, load vehicle data
    void init(GLFWwindow* window) noexcept override;

    // Update our state according to new user input.
    void update(GLFWwindow* window) noexcept override;

    // Delete resources created in init().
    void destroy() noexcept override;
};