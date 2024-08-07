#include <stdbool.h>
#include <memory.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include <common/int.h>
#include <common/file.h>
#include <common/logging.h>
#include <common/shader.h>
#include <common/primitives.h>
#include <common/gl_setup.h>

#include <vehicle.h>
#include <physfs_bundling.h>
#include "camera.h"
#include "gui_common.h"

// This doesn't enforce what the bound VAO is... make sure to only call it with
// the cube VAO bound.
void render_vehicle_bitmask(gui_state* gui, vehicle_bitmask* mask) {
    vec3s center = vehicle_find_center(gui->v);

    // Highest XYZ coords in the vehicle. We add 1 to include the highest
    // index, then 4 to avoid cutting off large parts with up to 4 cells radius
    vec3s max = glms_vec3_adds(glms_vec3_scale(center, PART_POS_SCALE), 5.0f);

    // Tranformation matrices
    mat4 pv = {0};
    camera_proj_view(gui, &pv);
    mat4 model = {0};
    glm_mat4_identity(model);

    // Loop over the bitmask & render everything within the vehicle bounds
    for (u16 i = 0; i < VEH_MAX_DIM && i < max.x; i++) {
        for (u16 j = 0; j < VEH_MAX_DIM && j < max.y; j++) {
            for (u16 k = 0; k < VEH_MAX_DIM && k < max.z; k++) {
                // Skip the next bits if they're all 0
                // Keeping this size small lets us "jump into action" faster when
                // a big empty space is encountered, even though a 64-bit compare
                // would be much more efficient.
                // TODO: Optimize this later? Maybe some sketchy __builtin_clz() stuff?
                u8 bits = 0;
                const u8 bit_size = sizeof(bits) * 8;
                const bool in_arr_bounds = (j < VEH_MAX_DIM - bit_size && i < VEH_MAX_DIM - bit_size);
                // We only need to run every (bit_size) iterations
                if (k % bit_size == 0 && in_arr_bounds) {
                    // When (bits) is a u8, this can just be an assignment
                    memcpy(&bits, &(*mask)[i][j][k / 8], sizeof(bits));
                    if (bits == 0) {
                        k += bit_size - 1;
                        continue;
                    }
                }


                bool part_present = vehiclemask_get_3d(mask, i, j, k);
                if (part_present) {
                    vec3 pos = {i, j, k};

                    // Move to the same position as the part rendering
                    pos[0] -= center.x;
                    pos[2] -= center.z;
                    glm_vec3_scale(pos, PART_POS_SCALE, pos);

                    mat4 pvm = {0};
                    glm_translate(model, pos);
                    glm_mat4_mul(pv, model, pvm); // Compute pvm

                    // TODO: The iteration seems to cause a heavy CPU bottleneck,
                    // but it's probably still worth doing a single instanced
                    // draw call instead of this.
                    glUniformMatrix4fv(gui->u_pvm, 1, GL_FALSE, (const float*)&pvm);
                    glDrawElements(GL_TRIANGLES, cube.idx_count, GL_UNSIGNED_SHORT, NULL);
                    glm_mat4_identity(model);
                }
            }
        }
    }
}


void update_edit_mode(gui_state* gui) {
    // Handle moving the selector box
    vec3s cam_view = glms_normalize(camera_facing());
    // Absolute value of camera vector
    vec3s cam_abs = {fabsf(cam_view.x), fabsf(cam_view.y), fabsf(cam_view.z)};

    s8 forward_diff = ((input.w && !gui->prev_input.w) - (input.s && !gui->prev_input.s));
    s8 side_diff = ((input.d && !gui->prev_input.d) - (input.a && !gui->prev_input.a));

    vec3s16 sel_box_prev = gui->sel_box;

    // Set our view direction to have a magnitude of 1 on the horizontal axis
    // we're facing the most strongly, and 0 in all other directions.
    vec3s horizontal_vec = {
        // zero if it's not the largest element, otherwise 1 or -1 depending on direction
        ((cam_view.x < 0) ? -1 : 1) * (cam_abs.x > cam_abs.z),
        0, // Vertical component ignored
        ((cam_view.z < 0) ? -1 : 1) * (cam_abs.z > cam_abs.x),
    };

    if (!input.control) {
        vec3s right_vec = {-horizontal_vec.z, 0, horizontal_vec.x};

        for (u8 i = 0; i < 3; i++) {
            gui->sel_box.raw[i] += forward_diff * horizontal_vec.raw[i];
            gui->sel_box.raw[i] += side_diff * right_vec.raw[i];
        }

        // Handle vertical movement
        gui->sel_box.y += (input.space && !gui->prev_input.space);
        gui->sel_box.y -= (input.shift && !gui->prev_input.shift);
    }
    else {

        // Update the bitmask(s) if needed
        if (forward_diff != 0 || side_diff != 0) {
            vehicle_rotate_selection(gui->v, gui->selected_mask, gui->vacancy_mask, forward_diff, side_diff, cam_view);
            update_vehiclemask(gui->v, gui->vacancy_mask, gui->selected_mask);
        }
    }


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
                vehicle_adjusted &= vehicle_move_part(gui->v, i, diff);
                update_vehiclemask(gui->v, gui->vacancy_mask, gui->selected_mask);

                // Check for overlaps and block the placement if needed
                if (vehicle_selection_overlap(gui->v, gui->vacancy_mask)) {
                    gui->sel_mode = SEL_BAD;
                }
                else {
                    gui->sel_mode = SEL_ACTIVE;
                }
            }
        }

        // Move the selection box to the part's new location, if it moved out
        // of bounds and forced the vehicle to be adjusted.
        gui->sel_box.x -= diff.x * vehicle_adjusted;
        gui->sel_box.y -= diff.y * vehicle_adjusted;
        gui->sel_box.z -= diff.z * vehicle_adjusted;
    }

    // Keep selection box in bounds
    gui->sel_box.x = CLAMP(0, gui->sel_box.x, 127);
    gui->sel_box.y = CLAMP(0, gui->sel_box.y, 127);
    gui->sel_box.z = CLAMP(0, gui->sel_box.z, 127);

    // Handle user trying to select a part (unless the selection has overlaps).
    if (input.e && !gui->prev_input.e && gui->sel_mode != SEL_BAD) {
        // Convert to vec3s8
        vec3s8 pos = {gui->sel_box.x, gui->sel_box.y, gui->sel_box.z};
        part_entry *p = part_by_pos(gui->v, pos);
        if (gui->sel_mode == SEL_ACTIVE) {
            // User pressed the button while moving parts, which means
            // we should put them down.
            vehicle_unselect_all(gui->v);
            update_vehiclemask(gui->v, gui->vacancy_mask, gui->selected_mask);
            gui->sel_mode = SEL_NONE; // Now you can start moving the parts
        }
        else {
            if (p->selected) {
                // User pressed the button while selecting parts on an
                // already-selected part, which means they want to start moving them.
                gui->sel_mode = SEL_ACTIVE;
                update_vehiclemask(gui->v, gui->vacancy_mask, gui->selected_mask);
            }
            else {
                // Select the part (unless it's a blank)
                if (p->id > 0) {
                    p->selected = true;
                }
                gui->sel_mode = SEL_NONE;
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
        // Toggle vsync
        gui->vsync = !gui->vsync;
        set_vsync(gui->vsync);
    }
    if (input.u && !gui->prev_input.u) {
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

bool gui_init(gui_state* gui) {
    // Initialize part bitmask
    update_vehiclemask(gui->v, gui->vacancy_mask, gui->selected_mask);

    char* vert = physfs_load_file("/src/editor/shader/vcolor.vert");
    char* frag = physfs_load_file("/src/editor/shader/vcolor.frag");
    if (vert == NULL || frag == NULL) {
        return false;
    }
    gui->vcolor_shader = program_compile_src(vert, frag);
    if (!shader_link_check(gui->vcolor_shader)) {
        free(vert);
        free(frag);
        return false;
    }
    free(vert);
    free(frag);

    // Get our uniform locations
    gui->u_pvm = glGetUniformLocation(gui->vcolor_shader, "pvm");
    gui->u_paint = glGetUniformLocation(gui->vcolor_shader, "paint");

    model_upload(&quad);
    model_upload(&cube);

    return true;
}

void gui_teardown(gui_state* gui) {
    glDeleteProgram(gui->vcolor_shader);
    glDeleteVertexArrays(1, &quad.vao);
    glDeleteBuffers(1, &quad.vbuf);
    glDeleteBuffers(1, &quad.ibuf);

    glDeleteVertexArrays(1, &cube.vao);
    glDeleteBuffers(1, &cube.vbuf);
    glDeleteBuffers(1, &cube.ibuf);
    free(gui->v);
    free(gui->vacancy_mask);
    free(gui->selected_mask);
}

