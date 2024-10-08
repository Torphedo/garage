#include <stddef.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include <common/logging.h>
#include <common/primitives.h>
#include <common/file.h>
#include <physfs_bundling.h>

#include "camera.h"
#include "vehicle_edit.h"
#include "editor.h"
#include "render_garage.h"

model get_or_load_model(garage_state* state, part_id id) {
    for (u8 i = 0; i < ARRAY_SIZE(state->models); i++) {
        part_model* cur = &state->models[i];
        if (cur->id == id) {
            return cur->model;
        }
        // We hit an empty space without finding our model. We'll try to load it
        else if (cur->model.vertices == NULL || cur->model.indices == NULL) {
            cur->id = id;
            // Path is on the stack, so we don't need to free it
            const char* obj_path = part_get_obj_path(id).str;
            u8* obj_data = physfs_load_file(obj_path);
            if (obj_data != NULL) {
                cur->model = obj_load(obj_data);
            }
            free(obj_data);

            if (cur->model.vertices == NULL || cur->model.indices == NULL) {
                LOG_MSG(error, "Failed to load \"%s\" (0x%X)\n\n", part_get_info(id).name, id);

                // It's not here and we couldn't load it. Fall back to the cube
                cur->model = cube;
                return cube;
            }
            model_upload(&cur->model);

            LOG_MSG(info, "Loaded \"%s\" from \"%s\" in %.2fKiB\n\n", part_get_info(id).name, obj_path, (float)model_size(cur->model) / 1024.0f);
            return cur->model;
        }
    }

    // Couldn't find it & we're out of space to add it... return placeholder cube
    return state->models[0].model;
}

garage_state garage_init(editor_state* editor) {
    const double time_start = glfwGetTime();
    garage_state state = {0};

    // ID 0 will just render a cube
    state.models[0] = (part_model){.id = 0, .model = cube};

    part_iterator iter = part_iterator_setup(*editor, SEARCH_ALL);
    while (!iter.done) {
        const part_entry* p = part_iterator_next(&iter);
        get_or_load_model(&state, p->id);
    }


    const double time_end = glfwGetTime();
    const double elapsed = time_end - time_start;
    if (elapsed > 1.0 / 1000) {
        LOG_MSG(debug, "Finished in %.3lfms\n", elapsed * 1000);
    }
    return state;
}

void garage_render(garage_state* state, editor_state* editor) {
    // We need to bind the shader program before uploading uniforms
    glUseProgram(editor->vcolor_shader);

    // All our matrices for rendering, only PVM is uploaded to GPU
    mat4 pvm = {0};
    mat4 pv = {0};
    float move_speed = editor->cam.move_speed;
    camera_proj_view(editor->cam, pv);
    // Put move speed back to normal
    editor->cam.move_speed = move_speed;

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
        const model m = get_or_load_model(state, p->id);
        // Don't paint parts with custom models
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

    vec3s pos = {0};
    vec4s color = {.a = 1.0f};

    // Get cursor position
    pos = vec3_from_vec3s16(editor->sel_box, PART_POS_SCALE);
    pos.x -= (center.x * PART_POS_SCALE);
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
        glUniform4fv(editor->u_paint, 1, (const float *) &color);
        glDrawElements(GL_TRIANGLES, cube.idx_count, GL_UNSIGNED_SHORT, NULL);
    }

    // Lock camera onto selection box during editing
    if (editor->mode == MODE_EDIT) {
        camera_set_target(&editor->cam, pos);
    }

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);
}

void garage_destroy(garage_state* state) {
    // Unload all part models
    for (u8 i = 0; i < ARRAY_SIZE(state->models); i++) {
        model* m = &state->models[i].model;

        // This is uninitialized, an unknown part, or a part with no model
        // falling back to the (static) cube model. There's nothing to free.
        if (state->models[i].id == 0 || m->indices == cube.indices) {
            continue;
        }

        glDeleteBuffers(1, &m->ibuf);
        glDeleteBuffers(1, &m->vbuf);
        glDeleteVertexArrays(1, &m->vao);
        free((void*)m->indices);
        free((void*)m->vertices);

        // Clear the pointers & OpenGL object values
        model empty = {0};
        *m = empty;
    }
}

