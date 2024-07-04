#include <stddef.h>
#include <stdio.h>
#include <memory.h>
#include <malloc.h>

#include <glad/glad.h>
#define STB_RECT_PACK_IMPLEMENTATION // force following include to generate implementation
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#define STB_DXT_IMPLEMENTATION
#include <stb_dxt.h>

#include <common/file.h>
#include <common/logging.h>
#include <common/image.h>
#include <common/utf8.h>
#include "shader.h"
#include "primitives.h"
#include "gui_common.h"

enum {
    TTF_BUF_SIZE = 1 << 20,
    TTF_TEX_WIDTH = 512,
    TTF_TEX_HEIGHT = 512,
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

const char* ttf_path = "ProFontIIxNerdFontMono-Regular.ttf";

typedef struct {
    gui_state* gui;
    u8* bc4_bitmap;
    stbtt_packedchar* packed_chars;
    stbtt_pack_context pack_ctx;

    gl_obj font_atlas;
    gl_obj shader;
    gl_obj vao;
}text_state;

void text_destroy(text_state* ctx) {
    if (ctx == NULL) {
        return;
    }
    glDeleteProgram(ctx->shader);
    glDeleteVertexArrays(1, &ctx->vao);
    glDeleteTextures(1, &ctx->font_atlas);

    stbtt_PackEnd(&ctx->pack_ctx);
    free(ctx->bc4_bitmap);
    free(ctx->packed_chars);
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

    u8* ttf_buffer = file_load(ttf_path);
    u8 bitmap[TTF_TEX_HEIGHT][TTF_TEX_WIDTH] = {0};
    if (ttf_buffer == NULL) {
        LOG_MSG(error, "Failed to load TTF!\n");
        text_destroy(state);
        return NULL;
    }
    stbtt_PackBegin(&state->pack_ctx, (unsigned char*)bitmap, TTF_TEX_WIDTH, TTF_TEX_HEIGHT, 0, 1, NULL);
    // stbtt_PackSetSkipMissingCodepoints(&state->pack_ctx, true);
    float font_size = ((float)TTF_TEX_WIDTH / 8);
    stbtt_PackFontRange(&state->pack_ctx, ttf_buffer, 0, font_size, FIRST_CHAR, NUM_CHAR, state->packed_chars);

    free(ttf_buffer);

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

    // Projection matrix
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    const float aspect = (float)mode->width / (float)mode->height;
    // mat4 projection = {0};
    // glm_perspective_rh_no(glm_rad(45), aspect, 0.1f, 1000.0f, projection);

    float cur_x = -1.0f;
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
        float delta_x = 0;
        float delta_y = 0;
        stbtt_GetPackedQuad(ctx->packed_chars, ctx->pack_ctx.width, ctx->pack_ctx.height, idx, &delta_x, &delta_y, &baked_quad, false);

        const float scale = 0.001f;
        glm_translate(model, (vec3){cur_x + (QUAD_SIZE * scale) + 0.01f, 0.7f, 0.2f});
        glm_scale(model, (vec3){scale, scale * aspect, scale});
        glm_rotate_x(model, glm_rad(90.0f), model);

        memcpy(&transforms[char_idx++], model, sizeof(model));

        // Update X pos
        cur_x += ((delta_x / ctx->pack_ctx.width) * 2);
        // printf("U+%X\n", codepoint);
    }

    glUseProgram(ctx->shader);
    glBindVertexArray(quad.vao);
    glUniformMatrix4fv(glGetUniformLocation(ctx->shader, "transforms"), num_chars, GL_FALSE, (const float*)transforms);
    glDrawElementsInstanced(GL_TRIANGLES, quad.idx_count, GL_UNSIGNED_SHORT, NULL, num_chars);

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

