#include <stddef.h>
#include <malloc.h>

#include <glad/glad.h>
#include <cglm/cglm.h>
#include "GLFW/glfw3.h"

#include "render_garage.h"
#include "shader.h"
#include "primitives.h"
#include "camera.h"
#include "gui_common.h"

typedef struct {
     gui_state* gui;

     renderer_state status;
}garage_state;

void render_cube(vec3s pos, vec4s color, float scale, mat4 pv, garage_state* state) {
    gui_state* gui = state->gui;
    mat4 model = {0};
    glm_mat4_identity(model);
    mat4 pvm = {0};

    glm_translate(model, (float*)&pos);
    glm_scale_uni(model, scale); // Draw the box a little larger than the part cubes
    glm_mat4_mul(pv, model, pvm); // Compute pvm
    glUniformMatrix4fv(gui->u_pvm, 1, GL_FALSE, (const float *) &pvm);

    // Upload paint color & draw
    glUniform4fv(gui->u_paint, 1, (const float *) &color);
    glDrawElements(GL_TRIANGLES, cube.idx_count, GL_UNSIGNED_SHORT, NULL);
}

void* garage_init(gui_state* gui) {
    garage_state* state = calloc(1, sizeof(*state));
    if (state == NULL) {
        LOG_MSG(info, "Couldn't allocate for renderer state!\n");
        return NULL;
    }
    state->gui = gui;

    state->status = STATE_OK;
    return state;
}

void garage_render(void* ctx) {
    garage_state* state = (garage_state*)ctx;
    gui_state* gui = state->gui;
    if (state->status != STATE_OK) {
        // Printing here would spam console, there should already be errors there
        return;
    }

    glEnable(GL_DEPTH_TEST);

    // Enable transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // We need to bind the shader program before uploading uniforms
    glUseProgram(gui->vcolor_shader);

    // All our matrices for rendering, only PVM is uploaded to GPU
    mat4 pvm = {0};
    mat4 pv = {0};
    camera_proj_view(gui, &pv);
    mat4 model = {0};
    glm_mat4_identity(model);

    glm_mat4_mul(pv, model, pvm); // Compute pvm
    glUniformMatrix4fv(gui->u_pvm, 1, GL_FALSE, (const float*)&pvm);

    // "Paint" the floor orange
    vec4 quad_paint = {1.0f, 0.5f, 0.2f, 1.0f};
    glUniform4fv(gui->u_paint, 1, (const float*)&quad_paint);

    // Draw the floor
    glBindVertexArray(quad.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad.ibuf);
    glDrawElements(GL_TRIANGLES, quad.idx_count, GL_UNSIGNED_SHORT, NULL);


    // Bind cube model
    glBindVertexArray(cube.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube.ibuf);

    // Draw all our parts
    vehicle* v = state->gui->v;
    vec3s center = vehicle_find_center(v);
    for (u16 i = 0; i < v->head.part_count; i++) {
        // Move the part
        vec3s pos = vec3_from_vec3u8(v->parts[i].pos, PART_POS_SCALE);
        pos.x -= (center.x * PART_POS_SCALE);
        pos.z -= (center.z * PART_POS_SCALE);

        // Upload paint color & draw
        vec4s paint_col = vec4_from_rgba8(v->parts[i].color);
        if (v->parts[i].selected) {
            paint_col.a /= 3;
        }
        // Render
        render_cube(pos, paint_col, 1.0f, pv, state);
    }

    // Draw the selection box in wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    vec3s pos = {0};
    vec4s color = {.a = 1.0f};

    // Draw green boxes around all selected parts as we move them
    if (state->gui->sel_mode != SEL_NONE) {
        for (u16 i = 0; i < v->head.part_count; i++) {
            // Only draw cubes around selected parts
            if (!v->parts[i].selected) {
                continue;
            }

            // Move the part
            pos = vec3_from_vec3u8(v->parts[i].pos, PART_POS_SCALE);
            pos.x -= (center.x * PART_POS_SCALE);
            pos.z -= (center.z * PART_POS_SCALE);

            // We could do this else-if in 1 line using a bool as an index, but
            // I just don't care enough.
            if (state->gui->sel_mode == SEL_BAD) {
                color.r = 1.0f;
            }
            else {
                color.g = 1.0f;
            }

            // Render
            render_cube(pos, color, 1.2f, pv, state);
        }
    }
    else {
        // Move the selection box
        pos = vec3_from_vec3s16(state->gui->sel_box, PART_POS_SCALE);
        pos.x -= (center.x * PART_POS_SCALE);
        pos.z -= (center.z * PART_POS_SCALE);
        render_cube(pos, color, 1.2f, pv, state);
    }

    // Lock camera onto selection box during editing
    if (state->gui->mode == MODE_EDIT) {
        // TODO: This locks to the last selected part while moving parts. We
        // might want to calculate the centerpoint of the selection and lock
        // to that instead.
        camera_set_target(pos);
    }

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void garage_destroy(void* ctx) {
    garage_state* state = (garage_state*)ctx;
    if (state->status == STATE_DESTROYED) {
        LOG_MSG(warning, "Avoided a double free, check your destroy() calls!\n");
        return;
    }

    // On the off chance we get called again and don't crash while trying to
    // read this already-freed status value, render() will exit early.
    state->status = STATE_DESTROYED;
    free(state);
}

renderer garage = {
    .init = garage_init,
    .render = garage_render,
    .destroy = garage_destroy,
};

