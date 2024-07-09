#include <stddef.h>
#include <stdio.h>
#include <memory.h>
#include <malloc.h>

#include <glad/glad.h>
#include <stb_rect_pack.h>
#include <stb_truetype.h>
#include <stb_dxt.h>

#include <common/file.h>
#include <common/logging.h>
#include <common/image.h>
#include <common/utf8.h>
#include "shader.h"
#include "primitives.h"
#include "gui_common.h"

// A vertex with position and texture coordinates
typedef struct {
    vec3 position;
    vec2 texcoord;
}tex_vertex;

// The same quad, but with texture coordinates instead of colors
const tex_vertex texquad_vertices[] = {
    {
        .position = {QUAD_SIZE, -1.5f, QUAD_SIZE},
        .texcoord = {1.0f, 1.0f},
    },
    {
        .position = {QUAD_SIZE, -1.5f, -QUAD_SIZE},
        .texcoord = {1.0f, 0},
    },
    {
        .position = {-QUAD_SIZE, -1.5f, -QUAD_SIZE},
        .texcoord = {0, 0},
    },
    {
        .position = {-QUAD_SIZE,  -1.5f, QUAD_SIZE},
        .texcoord = {0, 1.0f},
    }
};

model tex_quad = {
    .vert_count = ARRAY_SIZE(texquad_vertices),
    .idx_count = ARRAY_SIZE(quad_indices),
    .vertices = texquad_vertices,
    .indices = quad_indices,
};

enum {
    TTF_TEX_WIDTH = 512,
    TTF_TEX_HEIGHT = 560,
    BC4_BLOCK_SIZE = 8,

    // The lower bound and size of the Unicode range we support.
    // Unicode 0x20 - 0xFF should cover English, German, Spanish, etc.
    FIRST_CHAR = 0x20,
    NUM_CHAR = 0xDF,
};

const char vert_src[] = {
#include "shader/text.vert.h"
};

const char frag_src[] = {
#include "shader/text.frag.h"
};

const char* ttf_path = "ProFontIIxNerdFontPropo-Regular.ttf";

typedef struct {
    gui_state* gui;
    u8* bc4_bitmap;
    stbtt_packedchar* packed_chars;
    stbtt_pack_context pack_ctx;
    stbtt_fontinfo font_info;
    u8* ttf_data;

    gl_obj font_atlas;
    gl_obj shader;

    // Shader uniform locations
    gl_obj transforms;
    gl_obj texcoords;

    gl_obj vao;
}text_state;

void text_destroy(text_state* ctx) {
    if (ctx == NULL) {
        return;
    }
    glDeleteProgram(ctx->shader);
    glDeleteVertexArrays(1, &ctx->vao);
    glDeleteTextures(1, &ctx->font_atlas);

    glDeleteVertexArrays(1, &tex_quad.vao);
    glDeleteBuffers(1, &tex_quad.vbuf);
    // tex quad shares the regular quad's index buffer, no need to delete it

    stbtt_PackEnd(&ctx->pack_ctx);
    free(ctx->bc4_bitmap);
    free(ctx->packed_chars);
    free(ctx->ttf_data);
    free(ctx);
}

void* text_init(gui_state* gui) {
    text_state* state = calloc(1, sizeof(*state));
    if (state == NULL) {
        return NULL;
    }
    state->gui = gui;

    if (TTF_TEX_WIDTH % 4 != 0 || TTF_TEX_HEIGHT % 4 != 0) {
        LOG_MSG(error, "Assert fail, texture size isn't a multiple of block size!\n");
        free(state);
        return NULL;
    }

    state->packed_chars = calloc(1, sizeof(*state->packed_chars) * NUM_CHAR);

    state->ttf_data = file_load(ttf_path);
    u8 bitmap[TTF_TEX_HEIGHT][TTF_TEX_WIDTH] = {0};
    if (state->ttf_data == NULL) {
        LOG_MSG(error, "Failed to load TTF!\n");
        text_destroy(state);
        return NULL;
    }
    stbtt_InitFont(&state->font_info, state->ttf_data, 0);
    stbtt_PackBegin(&state->pack_ctx, (unsigned char*)bitmap, TTF_TEX_WIDTH, TTF_TEX_HEIGHT, 0, 1, NULL);
    // stbtt_PackSetSkipMissingCodepoints(&state->pack_ctx, true);
    float font_size = ((float)TTF_TEX_WIDTH / 8);
    stbtt_PackFontRange(&state->pack_ctx, state->ttf_data, 0, font_size, FIRST_CHAR, NUM_CHAR, state->packed_chars);

    u32 size = (TTF_TEX_HEIGHT * TTF_TEX_WIDTH) / 2;
    state->bc4_bitmap = calloc(1, size);
    if (state->bc4_bitmap == NULL) {
        LOG_MSG(error, "Failed to allocate %d bytes for font atlas\n", size);
        text_destroy(state);
        return NULL;
    }

    u32 output_pos = 0;
    for (u16 i = 0; i < TTF_TEX_HEIGHT; i += 4) { // Rows
        for (u16 j = 0; j < TTF_TEX_WIDTH; j+= 4) { // Columns
            u8 bc4_in[16] = {0};
            // Copy the 4 rows of 4 pixels each into the input buffer
            for (u8 k = 0; k < 4; k++) {
                memcpy(&bc4_in[4 * k], &bitmap[i + k][j], 4);
            }

            // Compress the block
            stb_compress_bc4_block(state->bc4_bitmap + output_pos, bc4_in);
            output_pos += BC4_BLOCK_SIZE;
        }
    }

    texture out = {
        .channels = 1,
        .data = state->bc4_bitmap,
        .width = TTF_TEX_WIDTH,
        .height = TTF_TEX_HEIGHT,
        .compressed = true,
        .fmt = BC4,
        .mip_level = 0,
    };
    img_write(out, "font_atlas.dds"); // For debugging

    // Upload font texture
    glGenTextures(1, &state->font_atlas);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, state->font_atlas);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RED_RGTC1, TTF_TEX_WIDTH, TTF_TEX_HEIGHT, 0, size, state->bc4_bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Compile shaders
    gl_obj vert_shader = shader_compile_src(vert_src, GL_VERTEX_SHADER);
    gl_obj frag_shader = shader_compile_src(frag_src, GL_FRAGMENT_SHADER);
    if (vert_shader == 0 || frag_shader == 0) {
        LOG_MSG(error, "Shader compilation failure!\n");
        text_destroy(state);
        return NULL;
    }

    // Create & link final shader program
    state->shader = glCreateProgram();
    glAttachShader(state->shader, vert_shader);
    glAttachShader(state->shader, frag_shader);
    glLinkProgram(state->shader);

    // Delete individual shader objects
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    if (!shader_link_check(state->shader)) {
        LOG_MSG(error, "Shader linker failure!\n");
        text_destroy(state);
        return NULL;
    }
    glUseProgram(state->shader); // Needed to set uniforms

    // Set our sampler to texture unit 0 for portability
    gl_obj atlas = glGetUniformLocation(state->shader, "font_atlas");
    state->transforms = glGetUniformLocation(state->shader, "transforms");
    state->texcoords = glGetUniformLocation(state->shader, "texcoords");
    glUniform1i(atlas, 0);

    // Upload our custom text rendering quad
    glGenBuffers(1, &tex_quad.vbuf);
    tex_quad.ibuf = quad.ibuf; // Re-use quad indices
    glGenVertexArrays(1, &tex_quad.vao);

    // Setup vertex buffer
    glBindVertexArray(tex_quad.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tex_quad.ibuf);
    glBindBuffer(GL_ARRAY_BUFFER, tex_quad.vbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tex_vertex) * tex_quad.vert_count, tex_quad.vertices, GL_STATIC_DRAW);

    // Create vertex layout
    glVertexAttribPointer(0, sizeof(vec3) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(tex_vertex), (void*)offsetof(tex_vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, sizeof(vec2) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(tex_vertex), (void*)offsetof(tex_vertex, texcoord));
    glEnableVertexAttribArray(1);

    // Unbind our buffers to avoid messing our state up
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return state;
}

void text_render(text_state* ctx) {
    if (ctx == NULL) {
        return; // Avoid stupid segfaults.
    }

    const char* text = "Unicöde!";

    u32 total_len = 0;
    u8 char_len = 0;
    u32 num_chars = 0;
    // Loop step is variable because of variable character size
    for (u32 i = 0; i < strlen(text); i += char_len) {
        // Only needed to handle the variable step
        utf8_codepoint(&text[i], &char_len);
        num_chars++;
        total_len += char_len;
    }
    char_len = 0; // Reset for next loop

    // Now that we know how many characters need drawing, we can allocate
    mat4* transforms = calloc(num_chars, sizeof(*transforms));
    if (transforms == NULL) {
        LOG_MSG(error, "Allocation failed for %d 4x4 matrices!\n", num_chars);
        return;
    }
    // First 2 coords are top-left UVs, last 2 are the bottom-right UVs.
    vec4* texcoords = calloc(num_chars, sizeof(*texcoords));
    if (texcoords == NULL) {
        LOG_MSG(error, "Allocation failed for %d-element vec4 array!\n", num_chars);
        return;
    }

    // Get aspect ratio
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    const float aspect = (float)mode->width / (float)mode->height;

    const float scale = 0.003f;
    float cur_x = -1.0f + (QUAD_SIZE * scale);
    float cur_y = 0;
    u32 char_idx = 0;
    for (u32 i = 0; i < total_len; i += char_len) {
        const u32 codepoint = utf8_codepoint(&text[i], &char_len);
        if (FIRST_CHAR > codepoint || codepoint > (FIRST_CHAR + NUM_CHAR)) {
            continue; // Outside the range in the font atlas, skip it.
        }
        const u32 idx = codepoint - FIRST_CHAR;
        mat4 model = {0};
        glm_mat4_identity(model);
        stbtt_aligned_quad baked_quad = {0};
        {
            float temp;
            // int idx = stbtt_FindGlyphIndex(&ctx->font_info, codepoint);
            stbtt_GetPackedQuad(ctx->packed_chars, ctx->pack_ctx.width, ctx->pack_ctx.height, idx, &temp, &temp, &baked_quad, 0);
        }
        int left_bearing = 0;
        int width = 0;
        stbtt_GetCodepointHMetrics(&ctx->font_info, codepoint, &width, &left_bearing);
        float delta_x = (float)width / (float)ctx->pack_ctx.width;

        glm_translate(model, (vec3){cur_x, 0.7f, 0.2f});
        glm_scale(model, (vec3){scale, scale * aspect, scale});
        glm_rotate_x(model, glm_rad(90.0f), model);

        texcoords[char_idx][0] = baked_quad.s0;
        texcoords[char_idx][1] = baked_quad.t0;
        texcoords[char_idx][2] = baked_quad.s1;
        texcoords[char_idx][3] = baked_quad.t1;
        memcpy(&transforms[char_idx++], model, sizeof(model));

        // Advance on X by character width + character gap
        cur_x += (delta_x * scale) + (QUAD_SIZE * 2 * scale);
        // printf("U+%X\n", codepoint);
    }

    // Upload transforms & render
    glUseProgram(ctx->shader);
    glBindVertexArray(tex_quad.vao);
    glUniformMatrix4fv(ctx->transforms, num_chars, GL_FALSE, (const float*)transforms);
    glUniform4fv(ctx->texcoords, num_chars, (const float*)texcoords);
    glDrawElementsInstanced(GL_TRIANGLES, tex_quad.idx_count, GL_UNSIGNED_SHORT, NULL, num_chars);

    free(transforms);

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

typedef void (*renderproc)(void*);
renderer text = {
    text_init,
    (renderproc)text_render,
    (renderproc)text_destroy,
};

