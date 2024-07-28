#include <stddef.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include <common/logging.h>
#include "primitives.h"
#include "camera.h"
#include "model.h"
#include "gui_common.h"
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
            cur->model = obj_load(obj_path);

            if (cur->model.vertices == NULL || cur->model.indices == NULL) {
                LOG_MSG(error, "Failed to load \"%s\" (0x%X)\n\n", part_get_info(id).name, id);

                // It's not here and we couldn't load it. Fall back to the cube
                cur->model = cube;
                return cube;
            }
            model_upload(&cur->model);

            LOG_MSG(info, "Loaded \"%s\" in %.2fKiB\n\n", part_get_info(id).name, (float)model_size(cur->model) / 1024.0f);
            return cur->model;
        }
    }

    // Couldn't find it & we're out of space to add it... return placeholder cube
    return state->models[0].model;
}

garage_state garage_init(gui_state* gui) {
    garage_state state = {0};

    // ID 0 will just render a cube
    state.models[0] = (part_model){.id = 0, .model = cube};

    for (u16 i = 0; i < gui->v->head.part_count; i++) {
        get_or_load_model(&state, gui->v->parts[i].id);
    }

    return state;
}

void garage_render(garage_state* state, gui_state* gui) {
    // We need to bind the shader program before uploading uniforms
    glUseProgram(gui->vcolor_shader);

    // All our matrices for rendering, only PVM is uploaded to GPU
    mat4 pvm = {0};
    mat4 pv = {0};
    camera_proj_view(gui, &pv);
    mat4 mdl = {0};
    glm_mat4_identity(mdl);

    glm_mat4_mul(pv, mdl, pvm); // Compute pvm
    glUniformMatrix4fv(gui->u_pvm, 1, GL_FALSE, (const float*)&pvm);

    // "Paint" the floor orange
    vec4 quad_paint = {1.0f, 0.5f, 0.2f, 1.0f};
    glUniform4fv(gui->u_paint, 1, (const float*)&quad_paint);

    // Draw the floor
    glBindVertexArray(quad.vao);
    glDrawElements(GL_TRIANGLES, quad.idx_count, GL_UNSIGNED_SHORT, NULL);

    // Draw all our parts
    vehicle* v = gui->v;
    vec3s center = vehicle_find_center(v);
    for (u16 i = 0; i < v->head.part_count; i++) {
        part_entry* p = &v->parts[i];
        // Move the part
        vec3s pos = vec3_from_vec3s8(p->pos, PART_POS_SCALE);
        pos.x -= (center.x * PART_POS_SCALE);
        pos.z -= (center.z * PART_POS_SCALE);

        // Upload paint color & draw
        vec4s paint_col = vec4_from_rgba8(p->color);
        if (v->parts[i].selected) {
            paint_col.a /= 3;
        }

        // Load a model for the part, if possible.
        model m = get_or_load_model(state, p->id);
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
        glUniformMatrix4fv(gui->u_pvm, 1, GL_FALSE, (const float *) &pvm);

        // Upload paint color & draw
        glUniform4fv(gui->u_paint, 1, (const float *) &paint_col);
        glDrawElements(GL_TRIANGLES, m.idx_count, GL_UNSIGNED_SHORT, NULL);
    }

    // Go back to the cube
    glBindVertexArray(cube.vao);

    // Draw the selection box in wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    vec3s pos = {0};
    vec4s color = {.a = 1.0f};

    // Draw green boxes around all selected parts as we move them
    // Set selection box color
    if (gui->sel_mode == SEL_BAD) {
        color.r = 1.0f;
    }
    else {
        color.g = 1.0f;
    }
    glUniform4fv(gui->u_paint, 1, (const float*)&color);
    render_vehicle_bitmask(gui, gui->selected_mask);

    // Get selection position
    pos = vec3_from_vec3s16(gui->sel_box, PART_POS_SCALE);
    pos.x -= (center.x * PART_POS_SCALE);
    pos.z -= (center.z * PART_POS_SCALE);

    if (gui->sel_mode == SEL_NONE) {
        color.r = 0;
        color.g = 0;
        // Render selection box
        mat4 model = {0};
        glm_mat4_identity(model);
        mat4 pvm = {0};

        glm_translate(model, (float*)&pos);
        glm_scale_uni(model, 1.2f); // Draw the box a little larger than the part cubes
        glm_mat4_mul(pv, model, pvm); // Compute pvm
        glUniformMatrix4fv(gui->u_pvm, 1, GL_FALSE, (const float *) &pvm);

        // Upload paint color & draw
        glUniform4fv(gui->u_paint, 1, (const float *) &color);
        glDrawElements(GL_TRIANGLES, cube.idx_count, GL_UNSIGNED_SHORT, NULL);
    }

    // Lock camera onto selection box during editing
    if (gui->mode == MODE_EDIT) {
        // TODO: This locks to the last selected part while moving parts. We
        // might want to calculate the centerpoint of the selection and lock
        // to that instead.
        camera_set_target(pos);
    }

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

