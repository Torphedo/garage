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

        gui.sel_box.y += (gui.mode == MODE_EDIT) * (input.space && !gui.prev_input.space);
        gui.sel_box.y -= (gui.mode == MODE_EDIT) * (input.shift && !gui.prev_input.shift);
        if (input.v && !gui.prev_input.v) {
            gui.cam_allow_vertical = !gui.cam_allow_vertical;
        }
        if (input.tab && !gui.prev_input.tab) {
            // Cycle through modes. Shift-Tab goes backwards.
            gui.mode = (gui.mode + (input.control ? -1 : 1)) % MODE_ENUM_MAX;
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
