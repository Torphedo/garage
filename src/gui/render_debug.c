#include <glad/glad.h>

#include "primitives.h"
#include "camera.h"
#include "gui_common.h"

typedef struct {
    gui_state* gui;
}debug_render_state;

void* debug_init(gui_state* gui) {
    debug_render_state* state = calloc(1, sizeof(*state));
    if (state != NULL) {
        state->gui = gui;
    }

    return state;
}

void debug_render(void* ctx) {
    debug_render_state* state = (debug_render_state *)ctx;
    gui_state* gui = (gui_state*) state->gui;


    // Bind our shader & buffers
    glUseProgram(gui->vcolor_shader);
    glBindVertexArray(cube.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube.ibuf);
    // Draw in wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    vec4s color = {.b = 1.0f, .a = 1.0f};
    glUniform4fv(gui->u_paint, 1, (const float*)&color);

    render_vehicle_bitmask(gui, gui->vacancy_mask);

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void debug_destroy(void* ctx) {
    free(ctx);
}

typedef void (*renderproc)(void*);
renderer dbg_view = {
    debug_init,
    (renderproc)debug_render,
    (renderproc)debug_destroy,
};

