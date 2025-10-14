#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include <common/logging.h>

extern "C" {
#include <primitives.h>
#include <physfs_bundling.h>

#include "timing_targets.h"
}

#include "editor.hxx"
#include "vehicle_edit.hxx"
#include "render_garage.hxx"

// Find/load a model to be rendered a model for a given part
model get_or_load_model(garage_state* state, part_id id) {
    for (u8 i = 0; i < ARRAY_SIZE(state->models); i++) {
        part_model* cur = &state->models[i];
        if (cur->id == id) {
            return cur->m;
        }
        // We hit an empty space without finding our model. We'll try to load it
        else if (cur->m.vertices == NULL || cur->m.indices == NULL) {
            cur->id = id;
            // Path is on the stack, so we don't need to free it
            const char* obj_path = part_get_obj_path(id).str;
            u8* obj_data = physfs_load_file(obj_path);
            if (obj_data != NULL) {
                cur->m = obj_load(obj_data);
            }
            free(obj_data);

            if (cur->m.vertices == NULL || cur->m.indices == NULL) {
                LOG_MSG(error, "Failed to load \"%s\" (0x%X)\n\n", part_get_info(id).name, id);

                // It's not here and we couldn't load it. Fall back to the cube
                cur->m= cube;
                return cube;
            }
            model_upload(&cur->m);

            LOG_MSG(info, "Loaded \"%s\" from \"%s\" in %.2fKiB\n\n", part_get_info(id).name, obj_path, (float)model_size(cur->m) / 1024.0f);
            return cur->m;
        }
    }

    // Couldn't find it & we're out of space to add it... return placeholder cube
    return state->models[0].m;
}

void garage_state::init(GLFWwindow* window) noexcept {
    const double time_start = glfwGetTime();

    // ID 0 will just render a cube
    models[0] = (part_model){
        .id = (part_id)0,
        .m = cube
    };

    part_iterator iter = part_iterator_setup(*editor, SEARCH_ALL);
    while (!iter.done) {
        const part_entry* p = part_iterator_next(&iter);
        get_or_load_model(this, (part_id)p->id);
    }

    DBG_ASSERT_PERF(time_start, 3);
}

void garage_state::render(GLFWwindow* window) noexcept {
    // We need to bind the shader program before uploading uniforms
    glUseProgram(editor->vcolor_shader);

    // All our matrices for rendering, only PVM is uploaded to GPU
    mat4 pvm = {0};
    mat4 pv = {0};
    editor->cam.proj_view(pv);

    mat4 mdl = {0};
    glm_mat4_identity(mdl);

    glm_mat4_mul(pv, mdl, pvm); // Compute pvm
    glUniformMatrix4fv(editor->u_pvm, 1, GL_FALSE, (const float*)&pvm);

    // "Paint" the floor orange
    vec4 quad_paint = {1.0f, 0.5f, 0.2f, 1.0f};
    glUniform4fv(editor->u_paint, 1, (const float*)&quad_paint);

    // Draw the floor
    glBindVertexArray(quad.vao);
    glDrawElements(GL_TRIANGLES, quad.idx_count, GL_UNSIGNED_SHORT, NULL);

    // Draw all our parts
    const vec3s center = vehicle_find_center(editor, SEARCH_ALL);
    part_iterator iter = part_iterator_setup(*editor, SEARCH_ALL);
    while (!iter.done) {
        const part_entry* p = part_iterator_next(&iter);

        // Move the part
        vec3s pos = vec3_from_vec3s8(p->pos, PART_POS_SCALE);
        pos.x -= (center.x * PART_POS_SCALE);
        pos.z -= (center.z * PART_POS_SCALE);

        // Upload paint color & draw
        vec4s paint_col = vec4_from_rgba8(p->color);
        if (list_contains(editor->selected_parts, (void*)p)) {
            paint_col.a /= 3;
        }

        // Load a model for the part, if possible.
        const model m = get_or_load_model(this, (part_id)p->id);
        // Don't paint parts with custom models, it'll make the vertex colors look weird.
        if (m.vao != cube.vao) {
            paint_col = (vec4s){.r = 1.0f, .g = 1.0f, .b = 1.0f, paint_col.a};
        }

        // Bind our part model and render
        glBindVertexArray(m.vao);
        mat4 model = {0};
        mat4 pvm = {0};
        glm_mat4_identity(model);

        // Apply translation & rotation from part data
        glm_translate(model, (float*)&pos);
        glm_rotate_x(model, p->rot[0], model);
        glm_rotate_y(model, p->rot[1], model);
        glm_rotate_z(model, p->rot[2], model);
        glm_mat4_mul(pv, model, pvm); // Compute pvm
        glUniformMatrix4fv(editor->u_pvm, 1, GL_FALSE, (const float *) &pvm);

        // Upload paint color & draw
        glUniform4fv(editor->u_paint, 1, (const float *) &paint_col);
        glDrawElements(GL_TRIANGLES, m.idx_count, GL_UNSIGNED_SHORT, NULL);
    }

    // Go back to the cube
    glBindVertexArray(cube.vao);

    // Draw the selection box in wireframe mode (no backface culling)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_CULL_FACE);


    // Get cursor position
    vec3s pos = vec3_from_vec3s16(editor->sel_box, PART_POS_SCALE);
    pos.x -= (center.x * PART_POS_SCALE); // Align onto part grid
    pos.z -= (center.z * PART_POS_SCALE);

    {
        // Render cursor box
        mat4 model = {0};
        glm_mat4_identity(model);
        mat4 pvm = {0};

        glm_translate(model, (float*)&pos);
        glm_scale_uni(model, 1.2f); // Draw the box a little larger than the part cubes
        glm_mat4_mul(pv, model, pvm); // Compute pvm
        glUniformMatrix4fv(editor->u_pvm, 1, GL_FALSE, (const float *) &pvm);

        // Upload paint color & draw
        const vec4s color = {.a = 1.0f};
        glUniform4fv(editor->u_paint, 1, (const float *) &color);
        glDrawElements(GL_TRIANGLES, cube.idx_count, GL_UNSIGNED_SHORT, NULL);
    }

    // Lock camera onto selection box during editing
    if (editor->mode == MODE_EDIT) {
        editor->cam.target = pos;
    }

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);
}

void garage_state::destroy() noexcept {
    // Unload all part models
    for (part_model& part : models) {

        // This is uninitialized, an unknown part, or a part with no model
        // falling back to the (static) cube model. There's nothing to free.
        if (part.id == 0 || part.m.indices == cube.indices) {
            continue;
        }

        glDeleteBuffers(1, &part.m.ibuf);
        glDeleteBuffers(1, &part.m.vbuf);
        glDeleteVertexArrays(1, &part.m.vao);
        free((void*)part.m.indices);
        free((void*)part.m.vertices);

        // Clear the pointers & OpenGL object values
        memset(&part.m, 0, sizeof(part.m));
    }
}
