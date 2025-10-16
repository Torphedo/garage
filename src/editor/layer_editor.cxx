#include <cstdio>
#include <memory.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include <common/logging.h>
#include <action/action.hxx>

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

using namespace action;

// This has less of a gamepad equivalent, but is like the space key
bool editor_state::vertical_up_rising_edge() const noexcept {
    // Sorry, this is a little confusing. The trigger's neutral position is -1,
    // with 1 being "fully pressed". So (deadzone - 1) is the neutral position
    // plus the deadzone.
    const float trigger_deadzone = (-1.0f) + deadzone;
    const bool gamepad = (input.RT > trigger_deadzone) && !(prev_input.RT > trigger_deadzone);

    const bool keyboard = rising_edge(KEY_SPACE);
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
    float stick_vec = input.LS.x / fabsf(input.LS.x);
    if (fabsf(input.LS.x) < deadzone || fabsf(prev_input.LS.x) > deadzone) {
        // If we're in the deadzone or were outside it last frame, don't count it.
        stick_vec = 0;
    }

    const s8 keyboard_diff = (s8)rising_edge(KEY_D) - rising_edge(KEY_A);

    const s8 result = CLAMP(-1, keyboard_diff + stick_vec, 1);
    return result;
}

// Unit direction vector of movement on the Y axis ("W/S" key or LS Y axis)
// Returns -1, 0, or 1.
s8 editor_state::move_y_rising_edge() const noexcept {
    // Dividing by itself gives 1, and using absolute value preserves sign.
    // This gives us -1 for any negative value, and 1 for any positive one.
    float stick_vec = input.LS.y / fabsf(input.LS.y);
    stick_vec = -stick_vec; // Y axis is the opposite sign of the intuitive way
    if (fabsf(input.LS.y) < deadzone || fabsf(prev_input.LS.y) > deadzone) {
        // If we're in the deadzone or were outside it last frame, don't count it.
        stick_vec = 0;
    }

    const s8 keyboard_diff = rising_edge(KEY_W) - rising_edge(KEY_S);

    const s8 result = CLAMP(-1, keyboard_diff + stick_vec, 1);
    return result;
}

void editor_state::update_edit_mode() noexcept {
    // Handle moving the selector box
    const vec3s cam_view = cam.facing();
    // Absolute value of camera vector
    const vec3s cam_abs = {fabsf(cam_view.x), fabsf(cam_view.y), fabsf(cam_view.z)};

    const s8 movediff_forward = move_y_rising_edge();
    const s8 movediff_side = move_x_rising_edge();
    s8 forward_diff = actions["up"].rising_edge() - actions["down"].rising_edge();
    s8 side_diff = actions["right"].rising_edge() - actions["left"].rising_edge();
    const s8 vertical_diff = vertical_up_rising_edge() - vertical_down_rising_edge();

    const bool gp_roll_right = rising_edge(GAMEPAD_BUTTON_RIGHT_BUMPER);
    const bool gp_roll_left = rising_edge(GAMEPAD_BUTTON_LEFT_BUMPER);
    const s8 roll_left = rising_edge(KEY_Z) + gp_roll_left;
    const s8 roll_right = rising_edge(KEY_C) + gp_roll_right;

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
            vehicle_rotate_selection(this, forward_diff, side_diff, roll_diff);

            // Check for overlaps and block the placement if needed
            if (vehicle_selection_overlap(*this)) {
                sel_mode = SEL_BAD;
            } else {
                sel_mode = SEL_ACTIVE;
            }
        }
    } else {
        const vec3s right_vec = {-horizontal_vec.z, 0, horizontal_vec.x};

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
        part_iterator iter(*this, SEARCH_SELECTED);
        while (!iter.done()) {
            const part_entry* p = iter.next();
            vec3s8 adjustment = {0};
            vehicle_move_part(*this, *p, diff, &adjustment);

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
    }

    // Find index of the part we're targeting
    const vec3s8 pos = {sel_box.x, sel_box.y, sel_box.z};
    const part_entry* p = part_by_pos(*this, pos, SEARCH_ALL);

    const bool select_button_pressed = actions.at("confirm").rising_edge();
    const bool unselect_button_pressed = rising_edge(KEY_R) || rising_edge(GAMEPAD_BUTTON_B);
    const bool delete_button_pressed = rising_edge(KEY_C) || rising_edge(GAMEPAD_BUTTON_Y);
    if (sel_mode != SEL_BAD && !rotation) {
        if (sel_mode == SEL_NONE) {
            if (unselect_button_pressed) {
                unselected_parts.push_back(*p);
                find_erase_pod(selected_parts, *p);
            }
            else if (delete_button_pressed) {
                // Try to delete it from both lists
                find_erase_pod(selected_parts, *p);
                find_erase_pod(unselected_parts, *p);
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
                sel_mode = SEL_NONE; // Now you can start moving the parts
            } else if (p->id != 0) {
                if (cell_is_selected(*this, p->pos)) {
                    // User pressed the button while selecting parts on an
                    // already-selected part, which means they want to start moving
                    // them.
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
    action::update_gamepad();

    // Update delta time and our last 2 frame times
    delta_time = one_frame_ago - two_frames_ago;
    two_frames_ago = one_frame_ago;
    one_frame_ago = glfwGetTime();

    static bool cursor_lock = false;
    update_mods(window); // Update input.shift, input.ctrl, etc.
    update_gamepad();

    if (rising_edge(MOUSE_MIDDLE) || rising_edge(GAMEPAD_BUTTON_R3)) {
        // Cycle through camera modes
        camera_mode mode = (camera_mode)((cam.mode + 1) % CAMERA_MODE_ENUM_MAX);
        // This function handles the special camera settings per mode
        cam.set_mode(mode);
    }

    // Allow infinite cursor movement when clicking to pan the camera
    if (input.mouse[MOUSE_LEFT]) {
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

    if (rising_edge(KEY_V)) {
        // Toggle vsync
        vsync = !vsync;
        set_vsync(vsync);
    }
    if (input.control && rising_edge(KEY_S)) {
        editor_save_to_file(*this, "vehicle.bin");
    }

    // Cycle if Tab or X are pressed
    bool cycle_mode = actions["mode_cycle"].rising_edge();
    if (cycle_mode && mode != MODE_MENU) {
        // Cycle through modes. Ctrl-Tab goes backwards.
        mode = (editor_mode)((mode + (input.control ? -1 : 1)) % 2);
    }
    if (actions["pause"].rising_edge()) {
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

    action::input_end_frame();
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

    // Copy part data into the dynamic list
    memcpy((void*)unselected_parts.data(), vehicle->parts, sizeof(*vehicle->parts) * vehicle->head.part_count);
    free(vehicle);
}

void editor_state::init(GLFWwindow* window) noexcept {
    const double time_start = glfwGetTime();

    // Start tracking input state & using virtual cursor positions.
    glfwSetKeyCallback(window, action::update_key);
    glfwSetCursorPosCallback(window, action::update_cursor);
    glfwSetScrollCallback(window, action::update_scroll);
    glfwSetMouseButtonCallback(window, action::update_mouse_button);

    actions["confirm"] = action::bind(action::KEY_E, action::GAMEPAD_BUTTON_A);
    actions["mode_cycle"] = action::bind(action::KEY_TAB, action::GAMEPAD_BUTTON_X);
    actions["pause"] = action::bind(action::KEY_ESCAPE, action::GAMEPAD_BUTTON_START);
    actions["up"] = action::bind(action::KEY_UP, action::GAMEPAD_BUTTON_DPAD_UP);
    actions["down"] = action::bind(action::KEY_DOWN, action::GAMEPAD_BUTTON_DPAD_DOWN);
    actions["left"] = action::bind(action::KEY_LEFT, action::GAMEPAD_BUTTON_DPAD_LEFT);
    actions["right"] = action::bind(action::KEY_RIGHT, action::GAMEPAD_BUTTON_DPAD_RIGHT);

    one_frame_ago = glfwGetTime();
    two_frames_ago = glfwGetTime();

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

    u_pvm = glGetUniformLocation(vcolor_shader, "pvm");
    u_paint = glGetUniformLocation(vcolor_shader, "paint");

    model_upload(&quad);
    model_upload(&cube);

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
}

