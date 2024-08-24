#include <string.h>
#include <stdio.h>

#include <glad/glad.h>
#include <physfs.h>

#include "common/int.h"
#include "common/list.h"
#include "common/logging.h"
#include "common/gl_setup.h"
#include "common/input.h"
#include "common/path.h"

#include "editor/render_garage.h"
#include "editor/render_debug.h"
#include "editor/render_text.h"
#include "editor/render_user.h"
#include "editor/editor.h"
#include "editor/camera.h"

#include "vehicle.h"
#include "parts.h"
#include "physfs_bundling.h"

int main(int argc, char** argv) {
    enable_win_ansi(); // Enable color & extra terminal features on Windows
    if (argc != 2) {
        LOG_MSG(error, "No input files.\n");
        LOG_MSG(info, "Usage: garage [vehicle file]\n");
        return 1;
    }

    // Setup PhysicsFS
    if (!setup_physfs(argv[0])) {
        return 1;
    }

    if (strcmp(argv[1], "--dump-assets") == 0) {
        dump_assets();
        return 0;
    }

    const char* path = argv[1]; // Give our first argument a convenient name
    vehicle* v = vehicle_load(path);
    if (v == NULL) {
        // Loader function will print the error message for us, so just exit
        return 1;
    }
    editor_state editor = {
        .v = v,
        .selected_parts = list_create(sizeof(u16) * v->head.part_count),
        .vsync = true,
        .vacancy_mask = calloc(1, sizeof(vehicle_bitmask)),
        .selected_mask = calloc(1, sizeof(vehicle_bitmask)),
        .cam = camera_default(),
    };
    if (editor.vacancy_mask == NULL || editor.selected_mask == NULL) {
        LOG_MSG(error, "Failed to alloc a vehicle bitmask\n");
        return 1;
    }

    GLFWwindow* window = setup_opengl(680, 480, "Garage Opener", ENABLE_DEBUG, GLFW_CURSOR_NORMAL, editor.vsync);
    if (window == NULL) {
        return 1;
    }

    // Prepare for rendering
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Enable transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    if (!editor_init(&editor)) {
        return 1;
    }
    if (!text_renderer_setup("bin/ProFontIIxNerdFontPropo-Regular.ttf")) {
        LOG_MSG(error, "Couldn't setup text renderer\n");
        return 1;
    }

    garage_state garage = garage_init(&editor);

    char fps_text[32] = "FPS: 0 [0.00ms]";
    text_state fps_display = text_render_prep(fps_text, sizeof(fps_text), 0.03f, (vec2){-1, 1});

    double one_frame_ago = glfwGetTime(); // Used to calculate delta time
    double two_frames_ago = glfwGetTime();
    float frame_times[10] = {0}; // For averaging frame times

    // Main loop for rendering, UI, etc.
    while (!glfwWindowShouldClose(window)) {
        // Update delta time and our last 2 frame times
        editor.delta_time = one_frame_ago - two_frames_ago;
        two_frames_ago = one_frame_ago;
        one_frame_ago = glfwGetTime();

        // Shift over all the frame times and add in the last one
        memmove(&frame_times[1], frame_times, sizeof(frame_times) - sizeof(*frame_times));
        frame_times[0] = editor.delta_time;

        // Poll for input
        glfwPollEvents();

        // All UI/navigation/keybinds are implemented here
        if (!editor_update_with_input(&editor, window)) {
            // Returns false when the program should exit (for quit keybind)
            break;
        }

        camera_update(&editor.cam, editor.delta_time);

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        garage_render(&garage, &editor);
        debug_render(&editor);
        ui_update_render(&editor);

        // Update FPS counter 4 times a second
        if (fmod(one_frame_ago, 0.25) < 0.01) {
            text_update_transforms(&fps_display);
        }
        text_render(fps_display);

        // If VSync is on, this will wait for the next screen refresh.
        glfwSwapBuffers(window);

        // Update the last frame's input to this frame's input
        editor.prev_input = input;

        // Calculate framerate from average frame time
        float avg_time = 0.0f;
        for (u8 i = 0; i < ARRAY_SIZE(frame_times); i++) {
            avg_time += frame_times[i];
        }
        avg_time /= ARRAY_SIZE(frame_times);
        const float framerate = 1.0f / avg_time;
        const float time_ms = avg_time * 1000;

        // Update FPS text (to be rendered next frame)
        snprintf(fps_text, sizeof(fps_text), "FPS: %.0f [%.2fms]", framerate, time_ms);
        // End frame
    }

    // Print vehicle details (mostly a leftover from old versions of this program)
    LOG_MSG(info, "\"");
    print_c16s(v->head.name); // We need a special function to portably print UTF-16
    printf("\" has %d parts & weighs %f\n", v->head.part_count, v->head.weight);
    for (u16 i = 0; i < v->head.part_count; i++) {
        LOG_MSG(info, "Part %d: 0x%x [%s] ", i, v->parts[i].id, part_get_info(v->parts[i].id).name);
        printf("@ (%d, %d, %d) ", v->parts[i].pos.x, v->parts[i].pos.y, v->parts[i].pos.z);
        printf("painted #%x%x%x", v->parts[i].color.r, v->parts[i].color.g, v->parts[i].color.b);
        printf(", modifier 0x%02hx\n", v->parts[i].modifier);
    }

    // Cleanup
    text_free(fps_display);
    garage_destroy(&garage);
    ui_teardown();

    text_renderer_cleanup();
    editor_teardown(&editor);
    glfwTerminate(); // Auto-closes the window if we exited via the quit button

    return 0;
}

