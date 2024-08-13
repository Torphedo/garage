#include <glad/glad.h>

#include <common/primitives.h>
#include "render_debug.h"

void debug_render(gui_state* gui) {
    // Bind our shader & buffers
    glUseProgram(gui->vcolor_shader);
    glBindVertexArray(cube.vao);
    // Draw in wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    vec4s color = {.b = 1.0f, .a = 1.0f};
    glUniform4fv(gui->u_paint, 1, (const float*)&color);

    render_vehicle_bitmask(gui, gui->vacancy_mask);

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

