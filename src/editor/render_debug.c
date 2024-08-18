#include <glad/glad.h>

#include <common/primitives.h>
#include "render_debug.h"

void debug_render(editor_state* editor) {
    // Bind our shader & buffers
    glUseProgram(editor->vcolor_shader);
    glBindVertexArray(cube.vao);
    // Draw in wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    vec4s color = {.b = 1.0f, .a = 1.0f};
    glUniform4fv(editor->u_paint, 1, (const float*)&color);

    render_vehicle_bitmask(editor, editor->vacancy_mask);

    // Draw green/red boxes around all selected parts as appropriate
    // Set selection box color
    if (editor->sel_mode == SEL_BAD) {
        color.r = 1.0f;
    } else {
        color.g = 1.0f;
    }
    color.b = 0.0f;
    glUniform4fv(editor->u_paint, 1, (const float*)&color);
    render_vehicle_bitmask(editor, editor->selected_mask);


    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
