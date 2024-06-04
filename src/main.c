#include <stdio.h>

#include <glad/glad.h>
#include <string.h>

#include "common/int.h"
#include "common/logging.h"

#include "gui/gl_setup.h"
#include "gui/render_garage.h"
#include "gui/render_debug.h"
#include "gui/input.h"
#include "gui/gui_common.h"

#include "vehicle.h"
#include "parts.h"

int main(int argc, char** argv) {
    enable_win_ansi(); // Enable color & extra terminal features on Windows
    // TODO: Write separate function to check all our struct sizes
    if (argc != 2) {
        LOG_MSG(error, "No input files.\n");
        LOG_MSG(info, "Usage: garage [vehicle file]");
        return 1;
    }
    char* path = argv[1]; // Give our first argument a convenient name
    vehicle* v = vehicle_load(path);
    if (v == NULL) {
        // Loader function will print the error message for us, so just exit
        return 1;
    }
    gui_state gui = {
        .v = v,
        .vsync = true,
        .vehiclemask = calloc(1, sizeof(vehicle_bitmask)),
    };
    if (gui.vehiclemask == NULL) {
        LOG_MSG(error, "Failed to alloc vehicle bitmask\n");
        return 1;
    }

    GLFWwindow* window = setup_opengl(680, 480, "Garage Opener", ENABLE_DEBUG, GLFW_CURSOR_NORMAL);
    if (window == NULL) {
        return 1;
    }

    // Prepare for rendering
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    if (!gui_init(&gui)) {
        return 1;
    }
    void* garage_ctx = garage.init(&gui);
    void* dbg_ctx = dbg_view.init(&gui);

    double one_frame_ago = glfwGetTime(); // Used to calculate delta time
    double two_frames_ago = glfwGetTime();
    float frame_times[10] = {0};

    // Main loop for rendering, UI, etc.
    while (!glfwWindowShouldClose(window)) {
        // Update delta time and our last 2 frame times
        gui.delta_time = one_frame_ago - two_frames_ago;
        two_frames_ago = one_frame_ago;
        one_frame_ago = glfwGetTime();

        // Shift over all the frame times and add in the last one
        memmove(&frame_times[1], frame_times, sizeof(frame_times) - sizeof(*frame_times));
        frame_times[0] = gui.delta_time;

        // Poll for input
        glfwPollEvents();

        // All UI/navigation/keybinds are implemented here
        if (!gui_update_with_input(&gui, window)) {
            // Returns false when the program should exit (for quit keybind)
            break;
        }

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        garage.render(garage_ctx);
        // dbg_view.render(dbg_ctx);
        glfwSwapBuffers(window); // Framebuffer swap won't happen until vsync
        glFinish(); // Wait for vsync before going to the next line

        // Update the last frame's input to this frame's input
        gui.prev_input = input;


        // Calculate framerate from average frame time
        float avg_time = 0.0f;
        for (u8 i = 0; i < ARRAY_SIZE(frame_times); i++) {
            avg_time += frame_times[i];
        }
        avg_time /= ARRAY_SIZE(frame_times);

        // Print debug info about this frame
        LOG_MSG(debug, "Last frame: %.3lfms\n", avg_time * 1000.0);
        LOG_MSG(debug, "FPS: %.1f\n", 1.0 / avg_time);
        // Erase the last 2 lines, so we can make it look like they constantly update in-place.
        printf("\033[1F\033[K\033[1F\033[K");
        // End frame
    }

    // Cleanup
    garage.destroy(garage_ctx);
    dbg_view.destroy(dbg_ctx);
    gui_teardown(&gui);
    glfwTerminate(); // Auto-closes the window if we exited via the quit button

    // Print vehicle details (mostly a leftover from old versions of this program)
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
