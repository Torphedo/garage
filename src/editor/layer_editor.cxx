#include <cstdio>
#include <memory.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include <common/logging.h>

extern "C" {
#include <common/gl/shader.h>
#include <common/gl/gl_setup.h>
#include <primitives.h>
#include <vehicle.h>
#include <physfs_bundling.h>
#include "timing_targets.h"
}

#include "layer_editor.hxx"
#include "utils.hxx"
#include "vehicle_edit.hxx"


// A "confirm" action (like A button)
bool editor_state::confirm_rising_edge() const noexcept {
    const bool keyboard = (input.e && !prev_input.e);
    const bool gamepad = (input.gp.a && !prev_input.gp.a);
    return keyboard || gamepad;
}

// A "cancel" action (like B button)
bool editor_state::cancel_rising_edge() const noexcept {
    const bool keyboard = (input.q && !prev_input.q);
    const bool gamepad = (input.gp.b && !prev_input.gp.b);
    return keyboard || gamepad;
}

// A "pause" action (like start button)
bool editor_state::pause_rising_edge() const noexcept {
    const bool keyboard = (input.escape && !prev_input.escape);
    const bool gamepad = (input.gp.start && !prev_input.gp.start);
    return keyboard || gamepad;
}

// An "up" action (like dpad)
bool editor_state::up_rising_edge() const noexcept {
    const bool keyboard = (input.up && !prev_input.up);
    const bool gamepad = (input.gp.up && !prev_input.gp.up);
    return keyboard || gamepad;
}

// A "down" action (like dpad)
bool editor_state::down_rising_edge() const noexcept {
    const bool keyboard = (input.down && !prev_input.down);
    const bool gamepad = (input.gp.down && !prev_input.gp.down);
    return keyboard || gamepad;
}

// A "left" action (like dpad)
bool editor_state::left_rising_edge() const noexcept {
    const bool keyboard = (input.left && !prev_input.left);
    const bool gamepad = (input.gp.left && !prev_input.gp.left);
    return keyboard || gamepad;
}

// A "right" action (like dpad)
bool editor_state::right_rising_edge() const noexcept {
    const bool keyboard = (input.right && !prev_input.right);
    const bool gamepad = (input.gp.right && !prev_input.gp.right);
    return keyboard || gamepad;
}

// This has less of a gamepad equivalent, but is like the space key
bool editor_state::vertical_up_rising_edge() const noexcept {
    // Sorry, this is a little confusing. The trigger's neutral position is -1,
    // with 1 being "fully pressed". So (deadzone - 1) is the neutral position
    // plus the deadzone.
    const float trigger_deadzone = (-1.0f) + deadzone;
    const bool gamepad = (input.RT > trigger_deadzone) && !(prev_input.RT > trigger_deadzone);

    const bool keyboard = (input.space && !prev_input.space);
    return keyboard || gamepad;
}

// This has less of a gamepad equivalent, but is like a shift or crouch key
bool editor_state::vertical_down_rising_edge() const noexcept {
    const float trigger_deadzone = (-1.0f) + deadzone;
    const bool gamepad = (input.LT > trigger_deadzone) && !(prev_input.LT > trigger_deadzone);

    const bool keyboard = (input.shift && !prev_input.shift);
    return keyboard || gamepad;
}

// Unit direction vector of movement on the X axis ("A/D" key or LS X axis)
// Returns -1, 0, or 1.
s8 editor_state::move_x_rising_edge() const noexcept {
    // Dividing by itself gives 1, and using absolute value preserves sign.
    // This gives us -1 for any negative value, and 1 for any positive one.
    float stick_vec = input.LS_x / fabsf(input.LS_x);
    if (fabsf(input.LS_x) < deadzone || fabsf(prev_input.LS_x) > deadzone) {
        // If we're in the deadzone or were outside it last frame, don't count it.
        stick_vec = 0;
    }

    const s8 keyboard_diff = (input.d && !prev_input.d) - (input.a && !prev_input.a);

    const s8 result = CLAMP(-1, keyboard_diff + stick_vec, 1);
    return result;
}

// Unit direction vector of movement on the Y axis ("W/S" key or LS Y axis)
// Returns -1, 0, or 1.
s8 editor_state::move_y_rising_edge() const noexcept {
    // Dividing by itself gives 1, and using absolute value preserves sign.
    // This gives us -1 for any negative value, and 1 for any positive one.
    float stick_vec = input.LS_y / fabsf(input.LS_y);
    stick_vec = -stick_vec; // Y axis is the opposite sign of the intuitive way
    if (fabsf(input.LS_y) < deadzone || fabsf(prev_input.LS_y) > deadzone) {
        // If we're in the deadzone or were outside it last frame, don't count it.
        stick_vec = 0;
    }

    const s8 keyboard_diff = (input.w && !prev_input.w) - (input.s && !prev_input.s);

    const s8 result = CLAMP(-1, keyboard_diff + stick_vec, 1);
    return result;
}



// This doesn't enforce what the bound VAO is... make sure to only call it with
// the cube VAO bound.
void editor_state::render_vehicle_bitmask(const vehicle_bitmask* mask) const noexcept {
    vec3s center = vehicle_find_center(this, SEARCH_ALL);

    // Highest XYZ coords in the vehicle. We add 1 to include the highest
    // index, then 4 to avoid cutting off large parts with up to 4 cells radius
    vec3s max = glms_vec3_adds(glms_vec3_scale(center, PART_POS_SCALE), 5.0f);

    // Tranformation matrices
    mat4 pv = {0};
    cam.proj_view(pv);

    // Disable backface culling, it doesn't make sense for a wireframe
    GLboolean culling_was_enabled = false;
    glGetBooleanv(GL_CULL_FACE, &culling_was_enabled);
    glDisable(GL_CULL_FACE);
    // Loop over the bitmask & render everything within the vehicle bounds
    for (u8 i = 0; i < VEH_MAX_DIM && i < max.x; i++) {
        for (u8 j = 0; j < VEH_MAX_DIM && j < max.y; j++) {
            for (u8 k = 0; k < VEH_MAX_DIM && k < max.z; k++) {
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


                const vec3s8 cell = {(s8)i, (s8)j, (s8)k};
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
                glUniformMatrix4fv(u_pvm, 1, GL_FALSE, (const float*)&pvm);
                glDrawElements(GL_TRIANGLES, cube.idx_count, GL_UNSIGNED_SHORT, NULL);
            }
        }
    }

    // Re-enable backface culling if needed
    if (culling_was_enabled == GL_TRUE) {
        glEnable(GL_CULL_FACE);
    }
}

void editor_state::update_edit_mode() noexcept {
    // Handle moving the selector box
    const vec3s cam_view = cam.facing();
    // Absolute value of camera vector
    const vec3s cam_abs = {fabsf(cam_view.x), fabsf(cam_view.y), fabsf(cam_view.z)};

    const s8 movediff_forward = move_y_rising_edge();
    const s8 movediff_side = move_x_rising_edge();
    s8 forward_diff = up_rising_edge() - down_rising_edge();
    s8 side_diff = right_rising_edge() - left_rising_edge();
    const s8 vertical_diff = vertical_up_rising_edge() - vertical_down_rising_edge();

    const bool gp_roll_right = input.gp.rb && !prev_input.gp.rb;
    const bool gp_roll_left = input.gp.lb && !prev_input.gp.lb;
    const s8 roll_left = (input.z && !prev_input.z) + gp_roll_left;
    const s8 roll_right = (input.c && !prev_input.c) + gp_roll_right;

    // Combine keyboard and gamepad inputs
    const s8 roll_diff = roll_right - roll_left;

    const bool rotation = ((forward_diff + side_diff + roll_diff) != 0);
    const vec3s8 sel_box_prev = sel_box;

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
            bool needed_adjust = vehicle_rotate_selection(this, forward_diff, side_diff, roll_diff);
            // Update vacancy if the rest of the vehicle moved
            if (needed_adjust) {
                update_vacancymask(*this);
            }
            update_selectionmask(*this);

            // Check for overlaps and block the placement if needed
            if (vehicle_selection_overlap(*this)) {
                sel_mode = SEL_BAD;
            } else {
                sel_mode = SEL_ACTIVE;
            }
        }
    } else {
        vec3s right_vec = {-horizontal_vec.z, 0, horizontal_vec.x};

        for (u8 i = 0; i < 3; i++) {
            sel_box.raw[i] += movediff_forward * horizontal_vec.raw[i];
            sel_box.raw[i] += movediff_side * right_vec.raw[i];
        }

        // Handle vertical movement
        sel_box.y += vertical_diff;
    }


    // Handle moving the selection, if applicable
    if (sel_mode != SEL_NONE && !vec3s8_eq(sel_box, sel_box_prev) && !rotation) {
        const vec3s8 diff = {
            .x = sel_box.x - sel_box_prev.x,
            .y = sel_box.y - sel_box_prev.y,
            .z = sel_box.z - sel_box_prev.z,
        };

        // Move all selected parts
        bool needed_adjust = false;
        part_iterator iter(*this, SEARCH_SELECTED);
        while (!iter.done()) {
            part_entry* p = iter.next();
            vec3s8 adjustment = {0};
            needed_adjust |= vehicle_move_part(*this, *p, diff, &adjustment);
            // Adjust the selection box if needed
            sel_box.x -= adjustment.x;
            sel_box.y -= adjustment.y;
            sel_box.z -= adjustment.z;

            // Check for overlaps and block the placement if needed
            if (vehicle_selection_overlap(*this)) {
                sel_mode = SEL_BAD;
            } else {
                sel_mode = SEL_ACTIVE;
            }
        }

        // When moving selection, we only need to update the selection mask
        // TODO: Make a separate function for moving selection that uses
        // bitshifts/memcpy() to shift the grid. Need to do testing to see if
        // that's actually any faster.
        update_selectionmask(*this);

        // If rest of the vehicle was moved, we need to update the other grid
        if (needed_adjust) {
            update_vacancymask(*this);
        }
    }

    // Find index of the part we're targeting
    const vec3s8 pos = {sel_box.x, sel_box.y, sel_box.z};
    const part_entry* p = part_by_pos(*this, pos, SEARCH_ALL);

    const bool select_button_pressed = confirm_rising_edge();
    const bool unselect_button_pressed = (input.r && !prev_input.r) || (input.gp.b && !prev_input.gp.b);
    const bool delete_button_pressed = (input.c && !prev_input.c) || (input.gp.y && !prev_input.gp.y);
    if (sel_mode != SEL_BAD && !rotation) {
        if (sel_mode == SEL_NONE) {
            if (unselect_button_pressed) {
                unselected_parts.push_back(*p);

                find_erase_pod(selected_parts, *p);
                update_selectionmask(*this);
                update_vacancymask(*this);
            }
            else if (delete_button_pressed) {
                // Try to delete it from both lists
                find_erase_pod(selected_parts, *p);
                find_erase_pod(unselected_parts, *p);
                update_selectionmask(*this);
                update_vacancymask(*this);
                v.part_count--;
            }
        }
        if (select_button_pressed) {
            // Handle user trying to select a part, unless the selection has overlaps
            if (sel_mode == SEL_ACTIVE) {
                // User pressed the button while moving parts, which means
                // we should put them down.
                concat(unselected_parts, selected_parts);
                selected_parts.clear();
                update_selectionmask(*this); // This will boil down to just clearing the grid
                update_vacancymask(*this); // Need to add those parts to vacancy grid
                sel_mode = SEL_NONE; // Now you can start moving the parts
            } else if (p->id != 0) {
                if (cell_is_selected(*this, p->pos)) {
                    // User pressed the button while selecting parts on an
                    // already-selected part, which means they want to start moving
                    // them. No change to the vacancy/selection grids.
                    sel_mode = SEL_ACTIVE;

                    // Set cursor to the selection center
                    const vec3s center = vehicle_find_center(this, SEARCH_SELECTED);
                    sel_box = (vec3s8){
                        floorf(center.x),
                        floorf(center.y),
                        floorf(center.z),
                    };
                } else {
                    // Select the part
                    selected_parts.push_back(*p);
                    find_erase_pod(unselected_parts, *p);
                    update_selectionmask(*this);
                    update_vacancymask(*this);
                    sel_mode = SEL_NONE;
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
    part_iterator iter(editor, SEARCH_ALL);
    while (!iter.done()) {
        part_entry part = *iter.next();
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
void editor_state::update(GLFWwindow* window) noexcept {
    const double time_start = glfwGetTime();

    // Update delta time and our last 2 frame times
    delta_time = one_frame_ago - two_frames_ago;
    two_frames_ago = one_frame_ago;
    one_frame_ago = glfwGetTime();

    static bool cursor_lock = false;
    update_mods(window); // Update input.shift, input.ctrl, etc.
    gamepad_update();

    bool change_camstyle = (input.click_middle && !prev_input.click_middle);
    change_camstyle |= (input.gp.r3 && !prev_input.gp.r3);
    if (change_camstyle) {
        // Cycle through camera modes
        camera_mode mode = (camera_mode)((cam.mode + 1) % CAMERA_MODE_ENUM_MAX);
        // This function handles the special camera settings per mode
        cam.set_mode(mode);
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

    if (input.v && !prev_input.v) {
        // Toggle vsync
        vsync = !vsync;
        set_vsync(vsync);
    }
    if (input.control && input.s && !prev_input.s) {
        editor_save_to_file(*this, "vehicle.bin");
    }

    // Cycle if Tab or X are pressed
    bool cycle_mode = (input.tab && !prev_input.tab) || (input.gp.x && !prev_input.gp.x);
    if (cycle_mode && mode != MODE_MENU) {
        // Cycle through modes. Ctrl-Tab goes backwards.
        mode = (editor_mode)((mode + (input.control ? -1 : 1)) % 2);
    }
    if (pause_rising_edge()) {
        if (mode == MODE_MENU) {
            mode = MODE_MOVCAM;
        } else {
            mode = MODE_MENU;
        }
    }

    if (mode == MODE_EDIT) {
        update_edit_mode();
        cam.move_speed = 0;
    }

    // Only allow the camera to move in certain modes
    camera camera_default;
    switch (mode) {
    case MODE_MOVCAM:
        cam.move_speed = camera_default.move_speed;
        cam.mouse_sens = camera_default.mouse_sens;
        break;
    case MODE_MENU:
        cam.move_speed = 0;
        cam.mouse_sens = 0;
        break;
    default:
        cam.move_speed = 0;
        cam.mouse_sens = camera_default.mouse_sens;
    }
    cam.update(delta_time);

    prev_input = input;
    DBG_ASSERT_PERF(time_start, 1);
}

editor_state::editor_state(const char* vehicle_path) noexcept {
    vehicle* vehicle = vehicle_load(vehicle_path);
    if (!vehicle) {
        LOG_MSG(error, "Failed to load vehicle from \"%s\"\n", vehicle_path);
        return;
    }

    // Init vehicle header & dynamic lists
    this->v = vehicle->head;

    // Start with enough memory to select all parts without resizing
    selected_parts.reserve(v.part_count);

    // Reserve and set size, so we can safely memcpy
    unselected_parts.resize(v.part_count);

    // Copy part data into the dynamic list and free the raw vehicle data
    memcpy((void*)unselected_parts.data(), vehicle->parts, sizeof(*vehicle->parts) * vehicle->head.part_count);
    free(vehicle);
}

void editor_state::init(GLFWwindow* window) noexcept {
    const double time_start = glfwGetTime();
    // Start tracking input state & using virtual cursor positions.
    glfwSetKeyCallback(window, input_update);
    glfwSetCursorPosCallback(window, cursor_update);
    glfwSetScrollCallback(window, scroll_update);
    glfwSetMouseButtonCallback(window, mouse_button_update);

    one_frame_ago = glfwGetTime();
    two_frames_ago = glfwGetTime();

    vacancy_mask = (vehicle_bitmask*)calloc(1, sizeof(vehicle_bitmask));
    selected_mask = (vehicle_bitmask*)calloc(1, sizeof(vehicle_bitmask));

    if (!vacancy_mask || !selected_mask) {
        LOG_MSG(error, "Failed to alloc a vehicle bitmask\n");
        return;
    }

    // Initialize part grids
    update_vacancymask(*this);
    update_selectionmask(*this);

    u8* vert = physfs_load_file("/src/editor/shader/vcolor.vert");
    u8* frag = physfs_load_file("/src/editor/shader/vcolor.frag");
    if (!vert || !frag) {
        LOG_MSG(error, "Failed to load one or both of the vertex color shader files\n");
        return;
    }
    vcolor_shader = program_compile_src((char*)vert, (char*)frag);
    free(vert);
    free(frag);
    if (!shader_link_check(vcolor_shader)) {
        LOG_MSG(error, "Shader linker error\n");
        return;
    }

    // Get our uniform locations
    u_pvm = glGetUniformLocation(vcolor_shader, "pvm");
    u_paint = glGetUniformLocation(vcolor_shader, "paint");

    model_upload(&quad);
    model_upload(&cube);

    init_result = true;
    DBG_ASSERT_PERF(time_start, 1);
}

void editor_state::destroy() noexcept {
    glDeleteProgram(vcolor_shader);
    glDeleteVertexArrays(1, &quad.vao);
    glDeleteBuffers(1, &quad.vbuf);
    glDeleteBuffers(1, &quad.ibuf);

    glDeleteVertexArrays(1, &cube.vao);
    glDeleteBuffers(1, &cube.vbuf);
    glDeleteBuffers(1, &cube.ibuf);
    free(vacancy_mask);
    free(selected_mask);
}

