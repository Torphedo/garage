#include <stddef.h>
#include <malloc.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include "render_garage.h"
#include "shader.h"
#include "input.h"
#include "camera.h"
#include "gl_types.h"
#include "GLFW/glfw3.h"

vertex quad_vertices[] = {
    { .position = {0.5f, 0.5f, 0.0f},
        .tex_coord = {1.0f, 1.0f}
    },
    {
        .position = {0.5f, -0.5f, 0.0f},
        .tex_coord = {1.0f, 0.0f}
    },
    {
        .position = {-0.5f, -0.5f, 0.0f},
        .tex_coord = {0.0f, 0.0f}
    },
    {
        .position = {-0.5f,  0.5f, 0.0f},
        .tex_coord = {0.0f, 1.0f}
    },
    { .position = {0.5f, 0.5f, 0.0f},
        .tex_coord = {1.0f, 1.0f}
    },
    {
        .position = {-0.5f, -0.5f, 0.0f},
        .tex_coord = {0.0f, 0.0f}
    }
};

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
     vehicle* v;
     gl_obj shader_prog;
     gl_obj vertex_array;
     gl_obj vertex_buffer;

     // Uniforms
     gl_obj u_model;
     gl_obj u_view;
     gl_obj u_proj;

     renderer_state status;
}garage_state;

void* garage_init(vehicle* v) {
    garage_state* state = calloc(1, sizeof(*state));
    if (state == NULL) {
        LOG_MSG(info, "Couldn't allocate for renderer state!\n");
        return NULL;
    }
    state->v = v;

    // Setup VAO to store our state
    state->vertex_array = 0;
    glGenVertexArrays(1, &state->vertex_array);
    glBindVertexArray(state->vertex_array);

    // Setup vertex buffer
    state->vertex_buffer = 0;
    glGenBuffers(1, &state->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);

    // Create vertex layout
    glVertexAttribPointer(0, sizeof(vec3f) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, sizeof(vec2f) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, tex_coord));
    glEnableVertexAttribArray(1);

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
    state->u_model = glGetUniformLocation(state->shader_prog, "model");
    state->u_view = glGetUniformLocation(state->shader_prog, "view");
    state->u_proj = glGetUniformLocation(state->shader_prog, "projection");

    // Unbind VAO to avoid other renderers messing our state up
    glBindVertexArray(0);

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

    // We need to se the shader program before uploading uniforms
    glUseProgram(state->shader_prog);

    // Upload projection matrix
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    mat4 projection = {0};
    glm_perspective_rh_no(glm_rad(45), (float)mode->width / (float)mode->height, 0.1f, 1000.0f, projection);
    state->u_proj = glGetUniformLocation(state->shader_prog, "projection");
    glUniformMatrix4fv(state->u_proj, 1, GL_FALSE, (const float*)&projection);

    // Upload camera matrix
    mat4 view = {0};
    glm_mat4_identity(view);

    camera_update(&view);
    state->u_view = glGetUniformLocation(state->shader_prog, "view");
    glUniformMatrix4fv(state->u_view, 1, GL_FALSE, (const float*)&view);

    // Default model matrix
    mat4 model = {0};
    glm_mat4_identity(model); // Create identity matrix
    glUniformMatrix4fv(state->u_model, 1, GL_FALSE, (const float*)&model);

    // Draw
    glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
    glBindVertexArray(state->vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(quad_vertices) / sizeof(*quad_vertices));

    // Unbind VAO
    glBindVertexArray(0);
}

void garage_destroy(void* ctx) {
    garage_state* state = (garage_state*)ctx;
    if (state->status == STATE_DESTROYED) {
        LOG_MSG(warning, "Avoided a double free, check your destroy() calls!\n");
        return;
    }

    glDeleteProgram(state->shader_prog);
    glDeleteVertexArrays(1, &state->vertex_array);
    glDeleteBuffers(1, &state->vertex_buffer);

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
