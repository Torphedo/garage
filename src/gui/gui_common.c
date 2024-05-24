#include <stdbool.h>

#include "gui_common.h"
#include "camera.h"

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
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        cursor_lock = false;
    }

    if (input.v && !gui->prev_input.v) {
        gui->cam_allow_vertical = !gui->cam_allow_vertical;
    }
    if (input.tab && !gui->prev_input.tab) {
        // Cycle through modes. Shift-Tab goes backwards.
        gui->mode = (gui->mode + (input.control ? -1 : 1)) % 2;
    }
    if (input.escape && !gui->prev_input.escape) {
        gui->mode = MODE_MENU;
    }

    // Handle moving the selector box
    if (gui->mode == MODE_EDIT) {
        vec3s cam_view = glms_normalize(camera_facing());
        vec3s sel_mov_multiplier = {0};
        s16 forward_diff = ((input.w && !gui->prev_input.w) - (input.s && !gui->prev_input.s));
        s16 side_diff = ((input.d && !gui->prev_input.d) - (input.a && !gui->prev_input.a));
        if (fabsf(cam_view.x) > fabsf(cam_view.z)) {
            sel_mov_multiplier.x = cam_view.x / fabsf(cam_view.x);
            sel_mov_multiplier.z = sel_mov_multiplier.x;
            gui->sel_box.x += forward_diff * sel_mov_multiplier.x;
            gui->sel_box.z += side_diff * sel_mov_multiplier.z;
        } else {
            sel_mov_multiplier.z = cam_view.z / fabsf(cam_view.z);
            sel_mov_multiplier.x = sel_mov_multiplier.z;
            gui->sel_box.z += forward_diff * sel_mov_multiplier.z;
            gui->sel_box.x += -side_diff * sel_mov_multiplier.x;
        }

        gui->sel_box.y += (input.space && !gui->prev_input.space);
        gui->sel_box.y -= (input.shift && !gui->prev_input.shift);

        part_entry* p = &gui->v->parts[gui->sel_idx];
        if (gui->sel_mode == SEL_ACTIVE && !vec3u8_eq_vec3s16(p->pos, gui->sel_box)) {
            vec3s16 diff = {
                .x = gui->sel_box.x - p->pos.x,
                .y = gui->sel_box.y - p->pos.y,
                .z = gui->sel_box.z - p->pos.z,
            };
            vehicle_move_part(gui->v, gui->sel_idx, diff);
            gui->sel_box = (vec3s16){
                .x = p->pos.x,
                .y = p->pos.y,
                .z = p->pos.z,
            };
        }

        // Handle user trying to select a part
        if (input.e && !gui->prev_input.e) {
            if (gui->sel_mode == SEL_ACTIVE) {
                gui->sel_idx = 0;
                gui->sel_mode = SEL_NONE;
            }
            else {
                // If it's out of bounds, we can just ignore this.
                if (gui->sel_box.x >= 0 && gui->sel_box.y >= 0 && gui->sel_box.z >= 0) {
                    // Convert to vec3u8
                    vec3u8 pos = {gui->sel_box.x, gui->sel_box.y, gui->sel_box.z};
                    s32 i = part_idx_by_pos(gui->v, pos);
                    if (i >= 0) {
                        gui->sel_idx = i;
                        gui->sel_mode = SEL_ACTIVE;
                    }
                }
            }
        }
    }


    if (input.m && !gui->prev_input.m) {
        vehicle_move_part(gui->v, 2, (vec3s16){-1, 0, -2});
    }

    return true;
}
