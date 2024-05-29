#include <stddef.h>
#include <malloc.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include "render_garage.h"
#include "shader.h"
#include "primitives.h"
#include "camera.h"
#include "gui_common.h"
#include "GLFW/glfw3.h"

typedef enum {
    STATE_NONE,
    STATE_INIT_FAIL,
    STATE_OK,
    STATE_DESTROYED,
}renderer_state;

const char* frag = {
#include "shader/fragment.h"
};

const char* vert = {
#include "shader/vertex.h"
};

typedef struct {
     gui_state* gui;
     gl_obj shader_prog;
     gl_obj quad_vao;
     gl_obj quad_vbuf;
     gl_obj quad_ibuf;

     gl_obj cube_vao;
     gl_obj cube_vbuf;
     gl_obj cube_ibuf;

     // Uniforms
     gl_obj u_pvm; // PVM matrix uniform
     gl_obj u_paint;

     renderer_state status;
}garage_state;

enum {
    PART_SCALE = 1,
    PART_POS_SCALE = 5,
    SEL_BOX_SIZE = (PART_POS_SCALE * 2),
};

void render_cube(vec3s pos, vec4s color, float scale, mat4 pv, garage_state* state) {
    mat4 model = {0};
    glm_mat4_identity(model);
    mat4 pvm = {0};

    glm_translate(model, (float*)&pos);
    glm_scale_uni(model, scale); // Draw the box a little larger than the part cubes
    glm_mat4_mul(pv, model, pvm); // Compute pvm
    glUniformMatrix4fv(state->u_pvm, 1, GL_FALSE, (const float *) &pvm);

    // Upload paint color & draw
    glUniform4fv(state->u_paint, 1, (const float *) &color);
    glDrawElements(GL_TRIANGLES, cube.idx_count, GL_UNSIGNED_SHORT, NULL);
}

void* garage_init(gui_state* gui) {
    garage_state* state = calloc(1, sizeof(*state));
    if (state == NULL) {
        LOG_MSG(info, "Couldn't allocate for renderer state!\n");
        return NULL;
    }
    state->gui = gui;

    // Compile shaders
    gl_obj vertex_shader = shader_compile_src(vert, GL_VERTEX_SHADER);
    gl_obj fragment_shader = shader_compile_src(frag, GL_FRAGMENT_SHADER);
    if (vertex_shader == 0 || fragment_shader == 0) {
        LOG_MSG(error, "Failed to compile shaders\n");
        state->status = STATE_INIT_FAIL;
        return state;
    }
    state->shader_prog = glCreateProgram();
    glAttachShader(state->shader_prog, vertex_shader);
    glAttachShader(state->shader_prog, fragment_shader);
    glLinkProgram(state->shader_prog);

    if (!shader_link_check(state->shader_prog)) {
        state->status = STATE_INIT_FAIL;
        return state;
    }

    // Free the shader objects
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Get our uniform locations
    state->u_pvm = glGetUniformLocation(state->shader_prog, "pvm");
    state->u_paint = glGetUniformLocation(state->shader_prog, "paint");

    // Setup VAO
    glGenVertexArrays(1, &state->quad_vao);
    glBindVertexArray(state->quad_vao);

    // Setup vertex buffer
    glGenBuffers(1, &state->quad_vbuf);
    glBindBuffer(GL_ARRAY_BUFFER, state->quad_vbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * quad.vert_count, quad.vertices, GL_STATIC_DRAW);

    // Setup index buffer
    glGenBuffers(1, &state->quad_ibuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->quad_ibuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * quad.idx_count, quad_indices, GL_STATIC_DRAW);

    // Create vertex layout
    glVertexAttribPointer(0, sizeof(vec3) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, sizeof(vec4) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, color));
    glEnableVertexAttribArray(1);

    // Cube setup
    // Setup VAO
    glGenVertexArrays(1, &state->cube_vao);
    glBindVertexArray(state->cube_vao);

    // Setup vertex buffer
    glGenBuffers(1, &state->cube_vbuf);
    glBindBuffer(GL_ARRAY_BUFFER, state->cube_vbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * cube.vert_count, cube.vertices, GL_STATIC_DRAW);

    // Setup index buffer
    glGenBuffers(1, &state->cube_ibuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->cube_ibuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * cube.idx_count, cube.indices, GL_STATIC_DRAW);

    // Create vertex layout
    glVertexAttribPointer(0, sizeof(vec3) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, sizeof(vec4) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, color));
    glEnableVertexAttribArray(1);


    // Unbind our buffers to avoid other renderers messing our state up
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    state->status = STATE_OK;
    return state;
}

void garage_render(void* ctx) {
    garage_state* state = (garage_state*)ctx;
    if (state->status != STATE_OK) {
        // Printing here would spam console, there should already be errors there
        return;
    }

    glEnable(GL_DEPTH_TEST);

    // Enable transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // We need to bind the shader program before uploading uniforms
    glUseProgram(state->shader_prog);

    // All our matrices for rendering, only PVM is uploaded to GPU
    mat4 pvm = {0};
    mat4 pv = {0};
    mat4 model = {0};
    glm_mat4_identity(model);

    // Pre-multiply the projection & view components of the PVM matrix
    {
        // Projection matrix
        mat4 projection = {0};
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        float aspect = (float)mode->width / (float)mode->height;
        glm_perspective_rh_no(glm_rad(45), aspect, 0.1f, 1000.0f, projection);

        // Camera matrix
        mat4 view = {0};
        camera_update(state->gui, &view);
        glm_mat4_mul(projection, view, pv);
    }

    glm_mat4_mul(pv, model, pvm); // Compute pvm
    glUniformMatrix4fv(state->u_pvm, 1, GL_FALSE, (const float*)&pvm);

    // "Paint" the floor the right color
    glUniform4fv(state->u_paint, 1, (const float*)&quad_paint);

    // Draw the floor
    glBindVertexArray(state->quad_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->quad_ibuf);
    glDrawElements(GL_TRIANGLES, quad.idx_count, GL_UNSIGNED_SHORT, NULL);


    // Bind cube model
    glBindVertexArray(state->cube_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->cube_ibuf);

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

    glDeleteProgram(state->shader_prog);
    glDeleteVertexArrays(1, &state->quad_vao);
    glDeleteBuffers(1, &state->quad_vbuf);
    glDeleteBuffers(1, &state->quad_ibuf);

    glDeleteVertexArrays(1, &state->cube_vao);
    glDeleteBuffers(1, &state->cube_vbuf);
    glDeleteBuffers(1, &state->cube_ibuf);

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

