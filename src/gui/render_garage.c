#include <stddef.h>
#include <malloc.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include "render_garage.h"
#include "shader.h"
#include "camera.h"
#include "gui_common.h"
#include "GLFW/glfw3.h"

#define QUAD_SIZE (32.0f)
vec4 quad_paint = {1.0f, 0.5f, 0.2f, 1.0f};
const vertex quad_vertices[] = {
    {
        .position = {QUAD_SIZE, -1.5f, QUAD_SIZE},
        .color = {1.0f, 1.0f, 1.0f, 1.0f}
    },
    {
        .position = {QUAD_SIZE, -1.5f, -QUAD_SIZE},
        .color = {1.0f, 1.0f, 1.0f, 1.0f}
    },
    {
        .position = {-QUAD_SIZE, -1.5f, -QUAD_SIZE},
        .color = {1.0f, 1.0f, 1.0f, 1.0f}
    },
    {
        .position = {-QUAD_SIZE,  -1.5f, QUAD_SIZE},
        .color = {1.0f, 1.0f, 1.0f, 1.0f}
    }
};

const u16 quad_indices[] = {
    0, 1, 2,
    3, 0, 2,
};

#define CUBE_SIZE (1.0f)
#define CUBE_COLOR {1.0f, 1.0f, 1.0f, 1.0f}
const vertex cube_vertices[] = {
    {
        .position = {CUBE_SIZE, CUBE_SIZE, CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {-CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {-CUBE_SIZE,  CUBE_SIZE, CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
        .color = CUBE_COLOR
    },
    {
        .position = {-CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE},
        .color = CUBE_COLOR
    }
};

const u16 cube_indices[] = {
        0, 2, 1,
        0, 2, 3,
        4, 6, 5,
        4, 6, 7,

        0, 5, 1,
        0, 5, 4,
        2, 7, 3,
        2, 7, 6,

        2, 5, 1,
        2, 5, 6,
        0, 7, 3,
        0, 7, 4,
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
     gui_state* gui;
     gl_obj shader_prog;
     gl_obj quad_vao;
     gl_obj quad_vbuf;
     gl_obj quad_ibuf;

     gl_obj cube_vao;
     gl_obj cube_vbuf;
     gl_obj cube_ibuf;

     // Uniforms
     gl_obj u_model;
     gl_obj u_view;
     gl_obj u_proj;
     gl_obj u_paint;

     renderer_state status;
}garage_state;

enum {
    PART_SCALE = 1,
    PART_POS_SCALE = 5,
    SEL_BOX_SIZE = (PART_POS_SCALE * 2),
};

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
    state->u_model = glGetUniformLocation(state->shader_prog, "model");
    state->u_view = glGetUniformLocation(state->shader_prog, "view");
    state->u_proj = glGetUniformLocation(state->shader_prog, "projection");
    state->u_paint = glGetUniformLocation(state->shader_prog, "paint");

    // Setup VAO
    glGenVertexArrays(1, &state->quad_vao);
    glBindVertexArray(state->quad_vao);

    // Setup vertex buffer
    glGenBuffers(1, &state->quad_vbuf);
    glBindBuffer(GL_ARRAY_BUFFER, state->quad_vbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);

    // Setup index buffer
    glGenBuffers(1, &state->quad_ibuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->quad_ibuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), &quad_indices, GL_STATIC_DRAW);

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
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), &cube_vertices, GL_STATIC_DRAW);

    // Setup index buffer
    glGenBuffers(1, &state->cube_ibuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->cube_ibuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), &cube_indices, GL_STATIC_DRAW);

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

    // We need to se the shader program before uploading uniforms
    glUseProgram(state->shader_prog);

    // Upload projection matrix
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    mat4 projection = {0};
    glm_perspective_rh_no(glm_rad(45), (float)mode->width / (float)mode->height, 0.1f, 1000.0f, projection);
    glUniformMatrix4fv(state->u_proj, 1, GL_FALSE, (const float*)&projection);

    // Upload camera matrix
    mat4 view = {0};
    glm_mat4_identity(view);

    camera_update(state->gui, &view);
    glUniformMatrix4fv(state->u_view, 1, GL_FALSE, (const float*)&view);

    // Default model matrix
    mat4 model = {0};
    glm_mat4_identity(model); // Create identity matrix
    glUniformMatrix4fv(state->u_model, 1, GL_FALSE, (const float*)&model);

    // "Paint" the floor the right color
    glUniform4fv(state->u_paint, 1, (const float*)&quad_paint);

    // Draw
    glBindBuffer(GL_ARRAY_BUFFER, state->quad_vbuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->quad_ibuf);
    glBindVertexArray(state->quad_vao);
    glDrawElements(GL_TRIANGLES, sizeof(quad_indices) / sizeof(*quad_indices), GL_UNSIGNED_SHORT, NULL);


    // Draw all our parts
    glBindBuffer(GL_ARRAY_BUFFER, state->cube_vbuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->cube_ibuf);
    glBindVertexArray(state->cube_vao);

    vehicle* v = state->gui->v;
    vec3s center = vehicle_find_center(v);
    for (u16 i = 0; i < v->head.part_count; i++) {
        // Move the part
        vec3s pos = {
            .x = ((float)v->parts[i].pos.x - center.x) * PART_POS_SCALE,
            .y = ((float)v->parts[i].pos.y) * PART_POS_SCALE,
            .z = ((float)v->parts[i].pos.z - center.z) * PART_POS_SCALE,
        };
        glm_translate(model, (float*)&pos);
        glUniformMatrix4fv(state->u_model, 1, GL_FALSE, (const float*)&model);

        // Upload paint color & draw
        vec4s paint_col = vec4_from_rgba8(v->parts[i].color);
        glUniform4fv(state->u_paint, 1, (const float*)&paint_col);
        glDrawElements(GL_TRIANGLES, sizeof(cube_indices) / sizeof(*cube_indices), GL_UNSIGNED_SHORT, NULL);
        glm_mat4_identity(model);
    }

    // Move the part
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    vec3s pos = {
            .x = ((float)state->gui->sel_box.x - center.x) * PART_POS_SCALE,
            .y = ((float)state->gui->sel_box.y) * PART_POS_SCALE,
            .z = ((float)state->gui->sel_box.z - center.z) * PART_POS_SCALE,
    };
    glm_translate(model, (float*)&pos);
    glm_scale_uni(model, 1.2f);
    glUniformMatrix4fv(state->u_model, 1, GL_FALSE, (const float*)&model);

    // Upload paint color & draw
    vec4s paint_col = { .a = 1.0f};
    glUniform4fv(state->u_paint, 1, (const float*)&paint_col);
    glDrawElements(GL_TRIANGLES, sizeof(cube_indices) / sizeof(*cube_indices), GL_UNSIGNED_SHORT, NULL);

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
