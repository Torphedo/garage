#include <stdbool.h>
#include <stdio.h>
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


// A "confirm" action (like A button)
bool confirm_rising_edge(editor_state editor) {
    const bool keyboard = (input.e && !editor.prev_input.e);
    const bool gamepad = (input.gp.a && !editor.prev_input.gp.a);
    return keyboard || gamepad;
}

// A "cancel" action (like B button)
bool cancel_rising_edge(editor_state editor) {
    const bool keyboard = (input.q && !editor.prev_input.q);
    const bool gamepad = (input.gp.b && !editor.prev_input.gp.b);
    return keyboard || gamepad;
}

// A "pause" action (like start button)
bool pause_rising_edge(editor_state editor) {
    const bool keyboard = (input.escape && !editor.prev_input.escape);
    const bool gamepad = (input.gp.start && !editor.prev_input.gp.start);
    return keyboard || gamepad;
}

// An "up" action (like dpad)
bool up_rising_edge(editor_state editor) {
    const bool keyboard = (input.up && !editor.prev_input.up);
    const bool gamepad = (input.gp.up && !editor.prev_input.gp.up);
    return keyboard || gamepad;
}

// A "down" action (like dpad)
bool down_rising_edge(editor_state editor) {
    const bool keyboard = (input.down && !editor.prev_input.down);
    const bool gamepad = (input.gp.down && !editor.prev_input.gp.down);
    return keyboard || gamepad;
}

// A "left" action (like dpad)
bool left_rising_edge(editor_state editor) {
    const bool keyboard = (input.left && !editor.prev_input.left);
    const bool gamepad = (input.gp.left && !editor.prev_input.gp.left);
    return keyboard || gamepad;
}

// A "right" action (like dpad)
bool right_rising_edge(editor_state editor) {
    const bool keyboard = (input.right && !editor.prev_input.right);
    const bool gamepad = (input.gp.right && !editor.prev_input.gp.right);
    return keyboard || gamepad;
}

// This has less of a gamepad equivalent, but is like the space key
bool vertical_up_rising_edge(editor_state editor) {
    // Sorry, this is a little confusing. The trigger's neutral position is -1,
    // with 1 being "fully pressed". So (deadzone - 1) is the neutral position
    // plus the deadzone.
    const float trigger_deadzone = (-1.0f) + deadzone;
    const bool gamepad = (input.RT > trigger_deadzone) && !(editor.prev_input.RT > trigger_deadzone);

    const bool keyboard = (input.space && !editor.prev_input.space);
    return keyboard || gamepad;
}

// This has less of a gamepad equivalent, but is like a shift or crouch key
bool vertical_down_rising_edge(editor_state editor) {
    const float trigger_deadzone = (-1.0f) + deadzone;
    const bool gamepad = (input.LT > trigger_deadzone) && !(editor.prev_input.LT > trigger_deadzone);

    const bool keyboard = (input.shift && !editor.prev_input.shift);
    return keyboard || gamepad;
}

// Unit direction vector of movement on the X axis ("A/D" key or LS X axis)
// Returns -1, 0, or 1.
s8 move_x_rising_edge(editor_state editor) {
    // Dividing by itself gives 1, and using absolute value preserves sign.
    // This gives us -1 for any negative value, and 1 for any positive one.
    float stick_vec = input.LS.x / fabsf(input.LS.x);
    if (fabsf(input.LS.x) < deadzone || fabsf(editor.prev_input.LS.x) > deadzone) {
        // If we're in the deadzone or were outside it last frame, don't count it.
        stick_vec = 0;
    }

    const s8 keyboard_diff = (input.d && !editor.prev_input.d) - (input.a && !editor.prev_input.a);

    const s8 result = CLAMP(-1, keyboard_diff + stick_vec, 1);
    return result;
}

// Unit direction vector of movement on the Y axis ("W/S" key or LS Y axis)
// Returns -1, 0, or 1.
s8 move_y_rising_edge(editor_state editor) {
    // Dividing by itself gives 1, and using absolute value preserves sign.
    // This gives us -1 for any negative value, and 1 for any positive one.
    float stick_vec = input.LS.y / fabsf(input.LS.y);
    stick_vec = -stick_vec; // Y axis is the opposite sign of the intuitive way
    if (fabsf(input.LS.y) < deadzone || fabsf(editor.prev_input.LS.y) > deadzone) {
        // If we're in the deadzone or were outside it last frame, don't count it.
        stick_vec = 0;
    }

    const s8 keyboard_diff = (input.w && !editor.prev_input.w) - (input.s && !editor.prev_input.s);

    const s8 result = CLAMP(-1, keyboard_diff + stick_vec, 1);
    return result;
}



// This doesn't enforce what the bound VAO is... make sure to only call it with
// the cube VAO bound.
void render_vehicle_bitmask(editor_state* editor, vehicle_bitmask* mask) {
    vec3s center = vehicle_find_center(editor, SEARCH_ALL);

    // Highest XYZ coords in the vehicle. We add 1 to include the highest
    // index, then 4 to avoid cutting off large parts with up to 4 cells radius
    vec3s max = glms_vec3_adds(glms_vec3_scale(center, PART_POS_SCALE), 5.0f);

    // Tranformation matrices
    mat4 pv = {0};
    camera_proj_view(editor->cam, pv);

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


                const vec3s8 cell = {i, j, k};
                const bool part_present = vehiclemask_get_3d(mask, cell);
                if (!part_present) {
                    continue;
                }
                vec3 pos = {i, j, k}; // lol can't reuse the vec3s8

                // Move to the same position as the part rendering
                pos[0] -= center.x;
                pos[2] -= center.z;
                glm_vec3_scale(pos, PART_POS_SCALE, pos);

                mat4 pvm = {0};
                mat4 model = {0};
                glm_mat4_identity(model);

                glm_translate(model, pos);
                // Scale up by an imperceptible amount to avoid Z-fighting
                glm_scale_uni(model, 1.0001f);
                glm_mat4_mul(pv, model, pvm); // Compute pvm

                // TODO: The iteration seems to cause a heavy CPU bottleneck,
                // but it's probably still worth doing a single instanced
                // draw call instead of this.
                glUniformMatrix4fv(editor->u_pvm, 1, GL_FALSE, (const float*)&pvm);
                glDrawElements(GL_TRIANGLES, cube.idx_count, GL_UNSIGNED_SHORT, NULL);
            }
        }
    }

    // Re-enable backface culling if needed
    if (culling_was_enabled == GL_TRUE) {
        glEnable(GL_CULL_FACE);
    }
}

void update_edit_mode(editor_state* editor) {
    // Handle moving the selector box
    const vec3s cam_view = glms_normalize(camera_facing(editor->cam));
    // Absolute value of camera vector
    const vec3s cam_abs = {fabsf(cam_view.x), fabsf(cam_view.y), fabsf(cam_view.z)};

    const s8 movediff_forward = move_y_rising_edge(*editor);
    const s8 movediff_side = move_x_rising_edge(*editor);
    s8 forward_diff = up_rising_edge(*editor) - down_rising_edge(*editor);
    s8 side_diff = right_rising_edge(*editor) - left_rising_edge(*editor);
    const s8 vertical_diff = vertical_up_rising_edge(*editor) - vertical_down_rising_edge(*editor);

    const bool gp_roll_right = input.gp.rb && !editor->prev_input.gp.rb;
    const bool gp_roll_left = input.gp.lb && !editor->prev_input.gp.lb;
    const s8 roll_left = (input.z && !editor->prev_input.z) + gp_roll_left;
    const s8 roll_right = (input.c && !editor->prev_input.c) + gp_roll_right;

    // Combine keyboard and gamepad inputs
    const s8 roll_diff = roll_right - roll_left;

    const bool rotation = ((forward_diff + side_diff + roll_diff) != 0);
    const vec3s16 sel_box_prev = editor->sel_box;

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
            editor->sel_box.raw[i] += movediff_forward * horizontal_vec.raw[i];
            editor->sel_box.raw[i] += movediff_side * right_vec.raw[i];
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
        part_iterator iter = part_iterator_setup(*editor, SEARCH_SELECTED);
        while (!iter.done) {
            part_entry* p = part_iterator_next(&iter);
            vec3s16 adjustment = {0};
            needed_adjust |= vehicle_move_part(editor, *p, diff, &adjustment);
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
    const part_entry* p = part_by_pos(editor, pos, SEARCH_ALL);

    const bool select_button_pressed = confirm_rising_edge(*editor);
    const bool unselect_button_pressed = (input.r && !editor->prev_input.r) || (input.gp.b && !editor->prev_input.gp.b);
    const bool delete_button_pressed = (input.c && !editor->prev_input.c) || (input.gp.y && !editor->prev_input.gp.y);
    if (editor->sel_mode != SEL_BAD && !rotation) {
        if (editor->sel_mode == SEL_NONE) {
            if (unselect_button_pressed) {
                list_add(&editor->unselected_parts, (void*)p);
                list_remove_val(&editor->selected_parts, (void*)p);
                update_selectionmask(editor);
                update_vacancymask(editor);
            }
            else if (delete_button_pressed) {
                // Try to delete it from both lists
                list_remove_val(&editor->selected_parts, (void*)p);
                list_remove_val(&editor->unselected_parts, (void*)p);
                update_selectionmask(editor);
                update_vacancymask(editor);
                editor->v.part_count--;
            }
        }
        if (select_button_pressed) {
            // Handle user trying to select a part, unless the selection has overlaps
            if (editor->sel_mode == SEL_ACTIVE) {
                // User pressed the button while moving parts, which means
                // we should put them down.
                list_merge(&editor->unselected_parts, editor->selected_parts);
                list_clear(&editor->selected_parts);
                update_selectionmask(editor); // This will boil down to just clearing the grid
                update_vacancymask(editor); // Need to add those parts to vacancy grid
                editor->sel_mode = SEL_NONE; // Now you can start moving the parts
            } else if (p->id != 0) {
                if (cell_is_selected(editor, p->pos)) {
                    // User pressed the button while selecting parts on an
                    // already-selected part, which means they want to start moving
                    // them. No change to the vacancy/selection grids.
                    editor->sel_mode = SEL_ACTIVE;

                    // Set cursor to the selection center
                    const vec3s center = vehicle_find_center(editor, SEARCH_SELECTED);
                    editor->sel_box = (vec3s16){
                        floorf(center.x),
                        floorf(center.y),
                        floorf(center.z),
                    };
                } else {
                    // Select the part
                    list_add(&editor->selected_parts, (void*)p);
                    list_remove_val(&editor->unselected_parts, (void*)p);
                    update_selectionmask(editor);
                    update_vacancymask(editor);
                    editor->sel_mode = SEL_NONE;
                }
            }
        }
    }
}

void editor_save_to_file(editor_state editor, const char* output_path) {
    FILE* f = fopen(output_path, "wb");
    if (f == NULL) {
        return;
    }
    // Save vehicle header. Byteswapping is OK b/c this is a copy of the data
    vehicle_header_byteswap(&editor.v);
    fwrite(&editor.v, sizeof(editor.v), 1, f);

    // Save each part
    part_iterator iter = part_iterator_setup(editor, SEARCH_ALL);
    while (!iter.done) {
        part_entry part = *part_iterator_next(&iter);
        part_byteswap(&part); // This is a copy, byteswapping is OK
        fwrite(&part, sizeof(part), 1, f);
    }

    // The game always seems to write 4 all-zero bytes at the end, so we do it
    // too just in case it's important.
    const u32 pad = 0;
    fwrite(&pad, sizeof(pad), 1, f);

    fclose(f);
}

// Update the GUI state according to new user input.
bool editor_update_with_input(editor_state* editor, GLFWwindow* window) {
    const double time_start = glfwGetTime();
    static bool cursor_lock = false;
    update_mods(window); // Update input.shift, input.ctrl, etc.
    gamepad_update();

    bool change_camstyle = (input.click_middle && !editor->prev_input.click_middle);
    change_camstyle |= (input.gp.r3 && !editor->prev_input.gp.r3);
    if (change_camstyle) {
        // Cycle through camera modes
        camera_mode mode = (editor->cam.mode + 1) % CAMERA_MODE_ENUM_MAX;
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
    if (input.control && input.s && !editor->prev_input.s) {
        editor_save_to_file(*editor, "vehicle.bin");
    }

    // Cycle if Tab or X are pressed
    bool cycle_mode = (input.tab && !editor->prev_input.tab) || (input.gp.x && !editor->prev_input.gp.x);
    if (cycle_mode && editor->mode != MODE_MENU) {
        // Cycle through modes. Ctrl-Tab goes backwards.
        editor->mode = (editor->mode + (input.control ? -1 : 1)) % 2;
    }
    if (pause_rising_edge(*editor)) {
        if (editor->mode == MODE_MENU) {
            editor->mode = MODE_MOVCAM;
        } else {
            editor->mode = MODE_MENU;
        }
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

    const double time_end = glfwGetTime();
    const double elapsed = time_end - time_start;
    if (elapsed > 1.0 / 1000) {
        LOG_MSG(debug, "Finished in %.3lfms\n", elapsed * 1000);
    }
    return true;
}

editor_state editor_init(const char* vehicle_path, GLFWwindow* window) {
    const double time_start = glfwGetTime();
    editor_state editor = {
        .vacancy_mask = calloc(1, sizeof(vehicle_bitmask)),
        .selected_mask = calloc(1, sizeof(vehicle_bitmask)),
        .cam = camera_default(),
        .window = window,
        .init_result = false, // Default to failure, this will only be set to success if all checks pass
    };
    if (editor.vacancy_mask == NULL || editor.selected_mask == NULL) {
        LOG_MSG(error, "Failed to alloc a vehicle bitmask\n");
        return editor;
    }

    vehicle* v = vehicle_load(vehicle_path);
    if (v == NULL) {
        LOG_MSG(error, "Failed to load vehicle from \"%s\"", vehicle_path);
        return editor;
    }

    // Init vehicle header & dynamic lists
    editor.v = v->head;
    // Start with enough memory to select all parts without resizing
    editor.selected_parts = list_create(sizeof(part_entry) * v->head.part_count, sizeof(part_entry));
    editor.unselected_parts = list_create(sizeof(part_entry) * v->head.part_count, sizeof(part_entry));
    if (editor.selected_parts.data == 0 || editor.unselected_parts.data == 0) {
        LOG_MSG(error, "Failed to allocate for dynamic lists\n");
        return editor;
    }


    // Copy part data into the dynamic list and free the raw vehicle data
    memcpy((void*)editor.unselected_parts.data, v->parts, sizeof(*v->parts) * v->head.part_count);
    editor.unselected_parts.end_idx = v->head.part_count;
    free(v);

    // Initialize part grids
    update_vacancymask(&editor);
    update_selectionmask(&editor);

    u8* vert = physfs_load_file("/src/editor/shader/vcolor.vert");
    u8* frag = physfs_load_file("/src/editor/shader/vcolor.frag");
    if (vert == NULL || frag == NULL) {
        LOG_MSG(error, "Failed to load one or both of the vertex color shader files\n");
        return editor;
    }
    editor.vcolor_shader = program_compile_src((char*)vert, (char*)frag);
    free(vert);
    free(frag);
    if (!shader_link_check(editor.vcolor_shader)) {
        LOG_MSG(error, "Shader linker error\n");
        return editor;
    }

    // Get our uniform locations
    editor.u_pvm = glGetUniformLocation(editor.vcolor_shader, "pvm");
    editor.u_paint = glGetUniformLocation(editor.vcolor_shader, "paint");

    model_upload(&quad);
    model_upload(&cube);

    editor.init_result = true;
    const double time_end = glfwGetTime();
    const double elapsed = time_end - time_start;
    if (elapsed > 1.0 / 1000) {
        LOG_MSG(debug, "Finished in %.3lfms\n", elapsed * 1000);
    }
    return editor;
}

void editor_teardown(editor_state* editor) {
    glDeleteProgram(editor->vcolor_shader);
    glDeleteVertexArrays(1, &quad.vao);
    glDeleteBuffers(1, &quad.vbuf);
    glDeleteBuffers(1, &quad.ibuf);

    glDeleteVertexArrays(1, &cube.vao);
    glDeleteBuffers(1, &cube.vbuf);
    glDeleteBuffers(1, &cube.ibuf);
    free((void*)editor->selected_parts.data);
    free((void*)editor->unselected_parts.data);
    free(editor->vacancy_mask);
    free(editor->selected_mask);
}

