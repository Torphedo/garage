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

    // All our matrices for rendering, only PVM is uploaded to GPU
    mat4 pv = {0};
    camera_proj_view(gui, &pv);
    mat4 model = {0};
    glm_mat4_identity(model);

    // Bind our shader & buffers
    glUseProgram(gui->vcolor_shader);
    glBindVertexArray(cube.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube.ibuf);
    // Draw in wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    vec3s center = vehicle_find_center(gui->v);
    // Highest XYZ coords in the vehicle. We add 1 to include the highest index.
    vec3s max = glms_vec3_adds(glms_vec3_scale(center, 2), 1.0f);
    vec4s color = {.b = 1.0f, .a = 1.0f};
    glUniform4fv(gui->u_paint, 1, (const float*)&color);

    // Loop over the bitmask
    for (u16 i = 0; i < UINT8_MAX + 1 && i < max.x; i++) {
        for (u16 j = 0; j < UINT8_MAX + 1 && j < max.y; j++) {
            for (u16 k = 0; k < UINT8_MAX + 1 && k < max.z; k++) {
                bool part_present = mask_get((u8*)&gui->partmask[i][j], k);
                if (part_present) {
                    vec3 pos = {i, j, k};

                    // Move to the same position as the part rendering
                    pos[0] -= center.x;
                    pos[2] -= center.z;
                    glm_vec3_scale(pos, PART_POS_SCALE, pos);

                    mat4 pvm = {0};
                    glm_translate(model, pos);
                    glm_mat4_mul(pv, model, pvm); // Compute pvm

                    glUniformMatrix4fv(gui->u_pvm, 1, GL_FALSE, (const float*)&pvm);
                    glDrawElements(GL_TRIANGLES, cube.idx_count, GL_UNSIGNED_SHORT, NULL);
                    glm_mat4_identity(model);
                }
            }
        }
    }

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void debug_destroy(void* ctx) {

}

typedef void (*renderproc)(void*);
renderer dbg_view = {
    debug_init,
    (renderproc)debug_render,
    (renderproc)debug_destroy,
};

