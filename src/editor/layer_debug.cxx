#include <glad/glad.h>

#include <primitives.h>
#include "layer_debug.hxx"
#include "vehicle_edit.hxx"

// This doesn't enforce what the bound VAO is... make sure to only call it with
// the cube VAO bound.
void render_part_boxes(const editor_state& editor, partsearch_type type) noexcept {
    const vec3s center = vehicle_find_center(&editor, SEARCH_ALL);

    // Tranformation matrices
    mat4 pv = {0};
    editor.cam.proj_view(pv);

    // Disable backface culling, it doesn't make sense for a wireframe
    GLboolean culling_was_enabled = false;
    glGetBooleanv(GL_CULL_FACE, &culling_was_enabled);
    glDisable(GL_CULL_FACE);

    const_part_iterator iter(editor, type);
    while (!iter.done) {
        const part_entry* part = iter.next();
        part_cell_iterator cell_iter(*part);
        while (!cell_iter.done) {
            const vec3s8 cell = cell_iter.next();
            vec3 pos = {float(cell.x), float(cell.y), float(cell.z)};

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

            glUniformMatrix4fv(editor.u_pvm, 1, GL_FALSE, (const float*)&pvm);
            glDrawElements(GL_TRIANGLES, cube.idx_count, GL_UNSIGNED_SHORT, NULL);
        }
    }

    // Re-enable backface culling if needed
    if (culling_was_enabled == GL_TRUE) {
        glEnable(GL_CULL_FACE);
    }
}

void layer_debug::render(GLFWwindow* window) noexcept {
    // Bind our shader & buffers
    glUseProgram(editor.vcolor_shader);
    glBindVertexArray(cube.vao);
    // Draw in wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    vec4s color = {.b = 1.0f, .a = 1.0f};
    glUniform4fv(editor.u_paint, 1, (const float*)&color);

    render_part_boxes(editor, SEARCH_UNSELECTED);

    // Draw green/red boxes around all selected parts as appropriate
    // Set selection box color
    if (editor.sel_mode == SEL_BAD) {
        color.r = 1.0f;
    } else {
        color.g = 1.0f;
    }
    color.b = 0.0f;
    glUniform4fv(editor.u_paint, 1, (const float*)&color);
    render_part_boxes(editor, SEARCH_SELECTED);

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
