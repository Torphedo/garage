#include <stdbool.h>
#include <memory.h>

#include <glad/glad.h>

#include <common/int.h>
#include <common/logging.h>

#include "camera.h"
#include "gui_common.h"
#include "shader.h"
#include "primitives.h"
#include "gl_setup.h"

const char* frag = {
#include "shader/fragment.h"
};

const char* vert = {
#include "shader/vertex.h"
};

void render_vehicle_bitmask(gui_state* gui, vehicle_bitmask* mask) {
    vec3s center = vehicle_find_center(gui->v);

    // Highest XYZ coords in the vehicle. We add 1 to include the highest
    // index, then 8 to avoid cutting off large parts with up to 4 cells radius
    vec3s max = glms_vec3_adds(glms_vec3_scale(center, 2), 5.0f);

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

    // Handle user trying to select a part (unless the selection is bad).
    if (input.e && !gui->prev_input.e && gui->sel_mode != SEL_BAD) {
        // If it's out of bounds, we can just ignore this.
        if (gui->sel_box.x >= 0 && gui->sel_box.y >= 0 && gui->sel_box.z >= 0) {
            // Convert to vec3s8
            vec3s8 pos = {gui->sel_box.x, gui->sel_box.y, gui->sel_box.z};
            part_entry *p = part_by_pos(gui->v, pos);
            if (p == NULL) {
                return;
            }

            if (p->selected) {
                switch (gui->sel_mode) {
                case SEL_ACTIVE:
                    // User pressed the button while moving parts, which means
                    // we should put them down.
                    vehicle_unselect_all(gui->v);
                    update_vehiclemask(gui->v, gui->vacancy_mask, gui->selected_mask);
                    gui->sel_mode = SEL_NONE; // Now you can start moving the parts
                    break;
                case SEL_BAD:
                    // User tried to place overlapping parts. Ignore.
                    break;
                case SEL_NONE:
                    // User pressed the button while selecting parts on an
                    // already-selected part, which means they want to start moving them.
                    gui->sel_mode = SEL_ACTIVE;
                    update_vehiclemask(gui->v, gui->vacancy_mask, gui->selected_mask);
                }
            } else if (p != NULL) {
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

    // Compile shaders
    gl_obj vertex_shader = shader_compile_src(vert, GL_VERTEX_SHADER);
    gl_obj fragment_shader = shader_compile_src(frag, GL_FRAGMENT_SHADER);
    if (vertex_shader == 0 || fragment_shader == 0) {
        LOG_MSG(error, "Failed to compile shaders\n");
        return false;
    }
    gui->vcolor_shader = glCreateProgram();
    glAttachShader(gui->vcolor_shader, vertex_shader);
    glAttachShader(gui->vcolor_shader, fragment_shader);
    glLinkProgram(gui->vcolor_shader);

    if (!shader_link_check(gui->vcolor_shader)) {
        return false;
    }

    // Free the shader objects
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

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

