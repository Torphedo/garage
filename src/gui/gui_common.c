#include <stdbool.h>

#include <common/int.h>
#include "camera.h"
#include "gui_common.h"

void update_edit_mode(gui_state* gui) {
    // Handle moving the selector box
    vec3s cam_view = glms_normalize(camera_facing());
    s8 forward_diff = ((input.w && !gui->prev_input.w) - (input.s && !gui->prev_input.s));
    s8 side_diff = ((input.d && !gui->prev_input.d) - (input.a && !gui->prev_input.a));

    vec3s16 sel_box_prev = gui->sel_box;

    // This is kind of spaghetti. Sorry.
    bool facing_z = fabsf(cam_view.z) > fabsf(cam_view.x);
    // Indices of the X or Z axis, depending on how we're facing
    u8 facing_axis = 2 * facing_z; // The axis we're facing
    u8 other_axis = 2 * !facing_z; // The axis we're not facing

    // Flip the sign of our forward/back movement as needed.
    // [sign] is -1 or 1, matching the sign of the direction we face.
    s8 sign = cam_view.raw[facing_axis] / fabsf(cam_view.raw[facing_axis]);
    gui->sel_box.raw[facing_axis] += forward_diff * sign;

    // Negate our sideways movement when facing Z.
    side_diff -= (2 * side_diff) * facing_z;
    gui->sel_box.raw[other_axis] += side_diff * sign;

    // Handle vertical movement
    gui->sel_box.y += (input.space && !gui->prev_input.space);
    gui->sel_box.y -= (input.shift && !gui->prev_input.shift);

    // Handle moving the selection, if applicable
    if (gui->sel_mode != SEL_NONE && !vec3s16_eq(gui->sel_box, sel_box_prev)) {
        vec3s16 diff = {
            .x = gui->sel_box.x - sel_box_prev.x,
            .y = gui->sel_box.y - sel_box_prev.y,
            .z = gui->sel_box.z - sel_box_prev.z,
        };

        bool vehicle_adjusted = false;
        // Move all selected parts
        for (u16 i = 0; i < gui->v->head.part_count; i++) {
            if (gui->v->parts[i].selected) {
                // vehicle_move_part() returns false when a part moves out of bounds
                vehicle_adjusted |= vehicle_move_part(gui->v, i, diff);
            }
        }

        // Move the selection box to the part's new location, if it moved out
        // of bounds and forced the vehicle to be adjusted.
        gui->sel_box.x -= diff.x * vehicle_adjusted;
        gui->sel_box.y -= diff.y * vehicle_adjusted;
        gui->sel_box.z -= diff.z * vehicle_adjusted;
    }

    // Handle user trying to select a part
    if (input.e && !gui->prev_input.e) {
        // If it's out of bounds, we can just ignore this.
        if (gui->sel_box.x >= 0 && gui->sel_box.y >= 0 && gui->sel_box.z >= 0) {
            // Convert to vec3u8
            vec3u8 pos = {gui->sel_box.x, gui->sel_box.y, gui->sel_box.z};
            part_entry *p = part_by_pos(gui->v, pos);
            if (p == NULL) {
                return;
            }

            if (p->selected) {
                if (gui->sel_mode == SEL_ACTIVE) {
                    // User pressed the button while moving parts, which means
                    // we should put them down.
                    vehicle_unselect_all(gui->v);
                    gui->sel_mode = SEL_NONE; // Now you can start moving the parts
                }
                else {
                    // User pressed the button while selecting parts on an
                    // already-selected part, which means they want to start moving them.
                    gui->sel_mode = SEL_ACTIVE;
                }
            } else {
                // Select the part (if it exists)
                if (p != NULL) {
                    p->selected = true;
                    gui->sel_mode = SEL_NONE;
                }
            }
        }
    }
}

// Update the GUI state according to new user input.
bool gui_update_with_input(gui_state* gui, GLFWwindow* window) {
    static bool cursor_lock = false;
    update_mods(window); // Update input.shift, input.ctrl, etc.

    // Quit when q is pressed
    if (input.q) {
        return false;
    }

    // Allow infinite cursor movement when clicking to pan the camera
    if (input.click_left) {
        if (!cursor_lock) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            // Get non-accelerated input if possible
            if (glfwRawMouseMotionSupported()) {
                glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            }
        }
        cursor_lock = true;
    }
    else if (cursor_lock) {
        // Disable when left click is released
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        cursor_lock = false;
    }

    if (input.v && !gui->prev_input.v) {
        gui->cam_allow_vertical = !gui->cam_allow_vertical;
    }
    if (input.tab && !gui->prev_input.tab) {
        // Cycle through modes. Ctrl-Tab goes backwards.
        gui->mode = (gui->mode + (input.control ? -1 : 1)) % 2;
    }
    if (input.escape && !gui->prev_input.escape) {
        gui->mode = MODE_MENU;
    }

    if (gui->mode == MODE_EDIT) {
        update_edit_mode(gui);
    }

    return true;
}
