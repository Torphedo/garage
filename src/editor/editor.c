#include <stdbool.h>
#include <memory.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include <common/int.h>
#include <common/logging.h>
#include <common/shader.h>
#include <common/primitives.h>
#include <common/gl_setup.h>

#include <vehicle.h>
#include <physfs_bundling.h>
#include "camera.h"
#include "editor.h"
#include "vehicle_edit.h"

// This doesn't enforce what the bound VAO is... make sure to only call it with
// the cube VAO bound.
void render_vehicle_bitmask(editor_state* editor, vehicle_bitmask* mask) {
    vec3s center = vehicle_find_center(editor->v);

    // Highest XYZ coords in the vehicle. We add 1 to include the highest
    // index, then 4 to avoid cutting off large parts with up to 4 cells radius
    vec3s max = glms_vec3_adds(glms_vec3_scale(center, PART_POS_SCALE), 5.0f);

    // Tranformation matrices
    mat4 pv = {0};
    camera_proj_view(editor->cam, pv);

    mat4 model = {0};
    glm_mat4_identity(model);

    // Disable backface culling, it doesn't make sense for a wireframe
    GLboolean culling_was_enabled = false;
    glGetBooleanv(GL_CULL_FACE, &culling_was_enabled);
    glDisable(GL_CULL_FACE);
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


                vec3s8 cell = {i, j, k};
                bool part_present = vehiclemask_get_3d(mask, cell);
                if (!part_present) {
                    continue;
                }
                vec3 pos = {i, j, k}; // lol can't reuse the vec3s8

                // Move to the same position as the part rendering
                pos[0] -= center.x;
                pos[2] -= center.z;
                glm_vec3_scale(pos, PART_POS_SCALE, pos);

                mat4 pvm = {0};
                glm_translate(model, pos);
                // Scale up by an imperceptible amount to avoid Z-fighting
                glm_scale_uni(model, 1.0001f);
                glm_mat4_mul(pv, model, pvm); // Compute pvm

                // TODO: The iteration seems to cause a heavy CPU bottleneck,
                // but it's probably still worth doing a single instanced
                // draw call instead of this.
                glUniformMatrix4fv(editor->u_pvm, 1, GL_FALSE, (const float*)&pvm);
                glDrawElements(GL_TRIANGLES, cube.idx_count, GL_UNSIGNED_SHORT, NULL);
                glm_mat4_identity(model);
            }
        }
    }

    // Re-enable backface culling if needed
    if (culling_was_enabled == GL_TRUE) {
        glEnable(GL_CULL_FACE);
    }

}


void update_edit_mode(editor_state* editor) {
    // Gamepad rotations directions
    const bool gp_forward = input.gp.up && !editor->prev_input.gp.up;
    const bool gp_back = input.gp.down && !editor->prev_input.gp.down;
    const bool gp_left = input.gp.left && !editor->prev_input.gp.left;
    const bool gp_right = input.gp.right && !editor->prev_input.gp.right;
    const bool gp_roll_right = input.gp.rb && !editor->prev_input.gp.rb;
    const bool gp_roll_left = input.gp.lb && !editor->prev_input.gp.lb;

    // I made a lot of variables here, but I'm just trying to preserve some
    // semblance of readability :)
    const bool gamepad_rotation = gp_forward || gp_back || gp_left || gp_right || gp_roll_left || gp_roll_right;

    const bool rotation = input.control || gamepad_rotation;
    // Handle moving the selector box
    const vec3s cam_view = glms_normalize(camera_facing(editor->cam));
    // Absolute value of camera vector
    const vec3s cam_abs = {fabsf(cam_view.x), fabsf(cam_view.y), fabsf(cam_view.z)};

    s8 forward_diff = 0;
    s8 side_diff = 0;
    s8 roll_diff = 0;
    s8 vertical_diff = 0;
    {
        // Sorry this is a nightmare... we subtract 1 from the deadzone because
        // triggers start at -1, with 1 being "fully pressed". So deadzone - 1
        // is the neutral position plus the deadzone.
        const bool LT = (input.LT > deadzone - 1) && !(editor->prev_input.LT > deadzone - 1);
        const bool RT = (input.RT > deadzone - 1) && !(editor->prev_input.RT > deadzone - 1);

        // Combine keyboard and gamepad inputs
        const s8 forward = (input.w && !editor->prev_input.w) + gp_forward;
        const s8 back = (input.s && !editor->prev_input.s) + gp_back;
        const s8 left = (input.a && !editor->prev_input.a) + gp_left;
        const s8 right = (input.d && !editor->prev_input.d) + gp_right;
        const s8 roll_left = (input.q && !editor->prev_input.q) + gp_roll_left;
        const s8 roll_right = (input.e && !editor->prev_input.e) + gp_roll_right;
        const s8 up = (input.space && !editor->prev_input.space) + RT;
        const s8 down = (input.shift && !editor->prev_input.shift) + LT;

        // Combine our directions into signed values on each axis
        forward_diff = forward - back;
        side_diff = right - left;
        roll_diff = roll_right - roll_left;
        vertical_diff = up - down;

        // Set anything below the deadzone to zero
        float LS_x = input.LS.x * (fabsf(input.LS.x) > deadzone);
        float LS_y = input.LS.y * (fabsf(input.LS.y) > deadzone);

        // Over deadzone positive -> 1, negative -> -1
        LS_x = CLAMP(-1, LS_x / deadzone, 1);
        LS_y = CLAMP(-1, LS_y / deadzone, 1);

        // Whether the axis was over the threshold last frame
        const bool last_LS_x = fabsf(editor->prev_input.LS.x) > deadzone;
        const bool last_LS_y = fabsf(editor->prev_input.LS.y) > deadzone;

        // Move if we're over the deadzone, but only if we weren't last frame.
        side_diff += LS_x * !last_LS_x;
        forward_diff -= LS_y * !last_LS_y;
    }

    vec3s16 sel_box_prev = editor->sel_box;

    // Set our view direction to have a magnitude of 1 on the horizontal axis
    // we're facing the most strongly, and 0 in all other directions.
    vec3s horizontal_vec = {
        // zero if it's not the largest element, otherwise 1 or -1 depending on direction
        ((cam_view.x < 0) ? -1 : 1) * (cam_abs.x > cam_abs.z),
        0, // Vertical component ignored
        ((cam_view.z < 0) ? -1 : 1) * (cam_abs.z > cam_abs.x),
    };

    if (rotation) {
        if (forward_diff != 0 || side_diff != 0 || roll_diff != 0) {
            bool needed_adjust = vehicle_rotate_selection(editor, forward_diff, side_diff, roll_diff);
            // Update vacancy if the rest of the vehicle moved
            if (needed_adjust) {
                update_vacancymask(editor);
            }
            update_selectionmask(editor);

            // Check for overlaps and block the placement if needed
            if (vehicle_selection_overlap(editor)) {
                editor->sel_mode = SEL_BAD;
            }
            else {
                editor->sel_mode = SEL_ACTIVE;
            }
        }
    } else {
        vec3s right_vec = {-horizontal_vec.z, 0, horizontal_vec.x};

        for (u8 i = 0; i < 3; i++) {
            editor->sel_box.raw[i] += forward_diff * horizontal_vec.raw[i];
            editor->sel_box.raw[i] += side_diff * right_vec.raw[i];
        }

        // Handle vertical movement
        editor->sel_box.y += vertical_diff;
    }


    // Handle moving the selection, if applicable
    if (editor->sel_mode != SEL_NONE && !vec3s16_eq(editor->sel_box, sel_box_prev) && !rotation) {
        vec3s16 diff = {
            .x = editor->sel_box.x - sel_box_prev.x,
            .y = editor->sel_box.y - sel_box_prev.y,
            .z = editor->sel_box.z - sel_box_prev.z,
        };

        // Move all selected parts
        bool needed_adjust = false;
        for (u32 i = 0; i < editor->selected_parts.end_idx; i++) {
            u16 idx = editor->selected_parts.data[i];
            vec3s16 adjustment = {0};
            needed_adjust |= vehicle_move_part(editor->v, idx, diff, &adjustment);
            // Move the selection box to the part's new location, if it moved out
            // of bounds and forced the vehicle to be adjusted.
            editor->sel_box.x -= adjustment.x;
            editor->sel_box.y -= adjustment.y;
            editor->sel_box.z -= adjustment.z;

            // Check for overlaps and block the placement if needed
            if (vehicle_selection_overlap(editor)) {
                editor->sel_mode = SEL_BAD;
            }
            else {
                editor->sel_mode = SEL_ACTIVE;
            }
        }

        // When moving selection, we only need to update the selection mask
        // TODO: Make a separate function for moving selection that uses
        // bitshifts/memcpy() to shift the grid. Need to do testing to see if
        // that's actually any faster.
        update_selectionmask(editor);

        // If rest of the vehicle was moved, we need to update the other grid
        if (needed_adjust) {
            update_vacancymask(editor);
        }
    }

    // Find index of the part we're targeting
    const vec3s8 pos = {editor->sel_box.x, editor->sel_box.y, editor->sel_box.z};
    const s32 idx = part_by_pos(editor->v, pos);

    const bool select_button_pressed = (input.e && !editor->prev_input.e) || (input.gp.a && !editor->prev_input.gp.a);
    const bool unselect_button_pressed = (input.r && !editor->prev_input.r) || (input.gp.b && !editor->prev_input.gp.b);
    if (unselect_button_pressed && editor->sel_mode == SEL_NONE && !rotation) {
        list_remove_val(&editor->selected_parts, idx);
        update_selectionmask(editor);
        update_vacancymask(editor);
    }
    else if (select_button_pressed && editor->sel_mode != SEL_BAD && !rotation) {
        // Handle user trying to select a part (unless the selection has overlaps).
        if (editor->sel_mode == SEL_ACTIVE) {
            // User pressed the button while moving parts, which means
            // we should put them down.
            list_clear(&editor->selected_parts);
            update_selectionmask(editor); // This will boil down to just clearing the grid
            update_vacancymask(editor); // Need to add those parts to vacancy grid
            editor->sel_mode = SEL_NONE; // Now you can start moving the parts
        } else if (idx != -1) {
            if (cell_is_selected(editor, editor->v->parts[idx].pos)) {
                // User pressed the button while selecting parts on an
                // already-selected part, which means they want to start moving
                // them. No change to the vacancy/selection grids.
                editor->sel_mode = SEL_ACTIVE;

                // Set cursor to the selection center
                editor->sel_box = vehicle_selection_center(editor);
            } else {
                // Select the part
                list_add(&editor->selected_parts, idx);
                update_selectionmask(editor);
                update_vacancymask(editor);
                editor->sel_mode = SEL_NONE;
            }
        }
    }
}

// Update the GUI state according to new user input.
bool editor_update_with_input(editor_state* editor, GLFWwindow* window) {
    static bool cursor_lock = false;
    update_mods(window); // Update input.shift, input.ctrl, etc.
    gamepad_update();

    bool change_camstyle = (input.f && !editor->prev_input.f);
    change_camstyle |= (input.gp.r3 && !editor->prev_input.gp.r3);
    if (change_camstyle) {
        // Cycle through camera modes
        camera_mode mode = editor->cam.mode + 1;
        mode %= CAMERA_MODE_ENUM_MAX;
        // This function handles the special camera settings per mode
        camera_set_mode(&editor->cam, mode);
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

    if (input.v && !editor->prev_input.v) {
        // Toggle vsync
        editor->vsync = !editor->vsync;
        set_vsync(editor->vsync);
    }

    editor_mode old_mode = editor->mode;
    // Cycle if Tab or X are pressed
    bool cycle_mode = (input.tab && !editor->prev_input.tab) || (input.gp.x && !editor->prev_input.gp.x);
    if (cycle_mode) {
        // Cycle through modes. Ctrl-Tab goes backwards.
        editor->mode = (editor->mode + (input.control ? -1 : 1)) % 2;
    }
    if (input.escape && !editor->prev_input.escape) {
        editor->mode = MODE_MENU;
    }

    if (editor->mode == MODE_EDIT) {
        update_edit_mode(editor);
        editor->cam.move_speed = 0;
    }

    // Only allow the camera to move in certain modes
    switch (editor->mode) {
    case MODE_MOVCAM:
        editor->cam.move_speed = camera_default().move_speed;
            editor->cam.mouse_sens = camera_default().mouse_sens;
        break;
    case MODE_MENU:
        editor->cam.move_speed = 0;
            editor->cam.mouse_sens = 0;
        break;
    default:
        editor->cam.move_speed = 0;
            editor->cam.mouse_sens = camera_default().mouse_sens;
    }

    return true;
}

bool editor_init(editor_state* editor) {
    // Initialize part grids
    update_vacancymask(editor);
    update_selectionmask(editor);

    u8* vert = physfs_load_file("/src/editor/shader/vcolor.vert");
    u8* frag = physfs_load_file("/src/editor/shader/vcolor.frag");
    if (vert == NULL || frag == NULL) {
        return false;
    }
    editor->vcolor_shader = program_compile_src((char*)vert, (char*)frag);
    if (!shader_link_check(editor->vcolor_shader)) {
        free(vert);
        free(frag);
        return false;
    }
    free(vert);
    free(frag);

    // Get our uniform locations
    editor->u_pvm = glGetUniformLocation(editor->vcolor_shader, "pvm");
    editor->u_paint = glGetUniformLocation(editor->vcolor_shader, "paint");

    model_upload(&quad);
    model_upload(&cube);

    return true;
}

void editor_teardown(editor_state* editor) {
    glDeleteProgram(editor->vcolor_shader);
    glDeleteVertexArrays(1, &quad.vao);
    glDeleteBuffers(1, &quad.vbuf);
    glDeleteBuffers(1, &quad.ibuf);

    glDeleteVertexArrays(1, &cube.vao);
    glDeleteBuffers(1, &cube.vbuf);
    glDeleteBuffers(1, &cube.ibuf);
    free(editor->v);
    free(editor->selected_parts.data);
    free(editor->vacancy_mask);
    free(editor->selected_mask);
}

