#include <stdio.h>

#include <glad/glad.h>

#include "common/int.h"
#include "common/logging.h"

#include "gui/gl_setup.h"
#include "gui/render_garage.h"
#include "gui/input.h"
#include "gui/camera.h"

#include "vehicle.h"
#include "part_ids.h"

int main(int argc, char** argv) {
    enable_win_ansi();
    check_part_entry(); // Sanity check for padding bugs
    if (argc != 2) {
        LOG_MSG(error, "No input files.\n");
        LOG_MSG(info, "Usage: garage [vehicle file]");
        return 1;
    }
    char* path = argv[1];
    vehicle* v = vehicle_load(path);
    if (v == NULL) {
        // Loader function will print the error message for us
        return 1;
    }
    gui_state gui = {.v = v};

    GLFWwindow* window = setup_opengl(680, 480, "Garage Opener", ENABLE_DEBUG, GLFW_CURSOR_NORMAL);
    if (window == NULL) {
        return 1;
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    void* garage_ctx = garage.init(&gui);

    bool cursor_lock = false;
    double one_frame_ago = glfwGetTime();
    double two_frames_ago = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        // Update delta time and our last 2 frame times
        gui.delta_time = one_frame_ago - two_frames_ago;
        two_frames_ago = one_frame_ago;
        one_frame_ago = glfwGetTime();

        // Poll for input & handle input data
        glfwPollEvents();
        update_mods(window);

        // Quit when q is pressed
        if (input.q) {
            break;
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

        if (input.v && !gui.prev_input.v) {
            gui.cam_allow_vertical = !gui.cam_allow_vertical;
        }
        if (input.tab && !gui.prev_input.tab) {
            // Cycle through modes. Shift-Tab goes backwards.
            gui.mode = (gui.mode + (input.control ? -1 : 1)) % 2;
        }
        if (input.escape && !gui.prev_input.escape) {
            gui.mode = MODE_MENU;
        }

        // Handle moving the selector box
        if (gui.mode == MODE_EDIT) {
            vec3s cam_view = glms_normalize(camera_facing());
            vec3s sel_mov_multiplier = {0};
            s16 forward_diff = ((input.w && !gui.prev_input.w) - (input.s && !gui.prev_input.s));
            s16 side_diff = ((input.d && !gui.prev_input.d) - (input.a && !gui.prev_input.a));
            if (fabsf(cam_view.x) > fabsf(cam_view.z)) {
                sel_mov_multiplier.x = cam_view.x / fabsf(cam_view.x);
                sel_mov_multiplier.z = sel_mov_multiplier.x;
                gui.sel_box.x += forward_diff * sel_mov_multiplier.x;
                gui.sel_box.z += side_diff * sel_mov_multiplier.z;
            } else {
                sel_mov_multiplier.z = cam_view.z / fabsf(cam_view.z);
                sel_mov_multiplier.x = sel_mov_multiplier.z;
                gui.sel_box.z += forward_diff * sel_mov_multiplier.z;
                gui.sel_box.x += -side_diff * sel_mov_multiplier.x;
            }

            gui.sel_box.y += (input.space && !gui.prev_input.space);
            gui.sel_box.y -= (input.shift && !gui.prev_input.shift);

            part_entry* p = &gui.v->parts[gui.sel_idx];
            if (gui.sel_mode == SEL_ACTIVE && !vec3u8_eq_vec3s16(p->pos, gui.sel_box)) {
                vec3s16 diff = {
                    .x = gui.sel_box.x - p->pos.x,
                    .y = gui.sel_box.y - p->pos.y,
                    .z = gui.sel_box.z - p->pos.z,
                };
                vehicle_move_part(gui.v, gui.sel_idx, diff);
                gui.sel_box = (vec3s16){
                    .x = p->pos.x,
                    .y = p->pos.y,
                    .z = p->pos.z,
                };
            }

            // Handle user trying to select a part
            if (input.e && !gui.prev_input.e) {
                if (gui.sel_mode == SEL_ACTIVE) {
                    gui.sel_idx = 0;
                    gui.sel_mode = SEL_NONE;
                }
                else {
                    // If it's out of bounds, we can just ignore this.
                    if (gui.sel_box.x >= 0 && gui.sel_box.y >= 0 && gui.sel_box.z >= 0) {
                        // Convert to vec3u8
                        vec3u8 pos = {gui.sel_box.x, gui.sel_box.y, gui.sel_box.z};
                        s32 i = part_idx_by_pos(gui.v, pos);
                        if (i >= 0) {
                            gui.sel_idx = i;
                            gui.sel_mode = SEL_ACTIVE;
                        }
                    }
                }
            }
        }


        if (input.m && !gui.prev_input.m) {
            vehicle_move_part(gui.v, 2, (vec3s16){-1, 0, -2});
        }

        LOG_MSG(debug, "Last frame: %.3lfms\n", gui.delta_time * 1000.0);
        LOG_MSG(debug, "FPS: %.1f\n", 1.0 / gui.delta_time);
        printf("\033[1F\033[K\033[1F\033[K");

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        garage.render(garage_ctx);
        glfwSwapBuffers(window);

        // Reset input
        gui.prev_input = input;
    }
    garage.destroy(garage_ctx);
    glfwTerminate();

    LOG_MSG(info, "\"");
    print_c16s(v->head.name); // We need a special function to portably print UTF-16
    printf("\" has %d parts\n", v->head.part_count);
    for (u16 i = 0; i < v->head.part_count; i++) {
        LOG_MSG(info, "Part %d: 0x%x [%s] ", i, v->parts[i].id, part_get_name(v->parts[i].id));
        printf("@ (%d, %d, %d) ", v->parts[i].pos.x, v->parts[i].pos.y, v->parts[i].pos.z);
        printf("painted #%x%x%x", v->parts[i].color.r, v->parts[i].color.g, v->parts[i].color.b);
        printf(", modifier 0x%02hx\n", v->parts[i].modifier);
    }

    return 0;
}
