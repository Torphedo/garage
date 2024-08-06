#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_truetype.h>
#include <stb_dxt.h>
#include <physfs.h>

#include <common/file.h>
#include <common/logging.h>
#include <common/image.h>
#include <common/utf8.h>
#include <common/shader.h>
#include <common/primitives.h>

#include "render_text.h"

// A vertex with position and texture coordinates
typedef struct {
    vec3 position;
    vec2 texcoord;
}tex_vertex;

enum {
    TTF_TEX_WIDTH = 512,
    TTF_TEX_HEIGHT = 512,
    BC4_BLOCK_SIZE = 8,

    // The lower bound and size of the Unicode range we support.
    // Unicode 0x20 - 0xFF should cover English, German, Spanish, etc.
    FIRST_CHAR = 0x20,
    NUM_CHAR = 0xDF,
};

// Regular quad, but with texture coordinates instead of colors
const tex_vertex texquad_vertices[] = {
    {
        .position = {0.5f, -1.5f, 0.5f},
        .texcoord = {1.0f, 1.0f},
    },
    {
        .position = {0.5f, -1.5f, -0.5f},
        .texcoord = {1.0f, 0},
    },
    {
        .position = {-0.5f, -1.5f, -0.5f},
        .texcoord = {0, 0},
    },
    {
        .position = {-0.5f,  -1.5f, 0.5f},
        .texcoord = {0, 1.0f},
    }
};

model tex_quad = {
    .vert_count = ARRAY_SIZE(texquad_vertices),
    .idx_count = ARRAY_SIZE(quad_indices),
    .vertices = texquad_vertices,
    .indices = quad_indices,
};

bool initialized = false;
stbtt_packedchar packed_chars[NUM_CHAR];
stbtt_pack_context pack_ctx;
s32 font_height;
s32 font_ascent;

// OpenGL objects
gl_obj font_atlas; // BC4 font atlas texture
gl_obj shader;
gl_obj u_transforms;
gl_obj u_texcoords;

bool text_renderer_setup(const char* ttf_path) {
    if (TTF_TEX_WIDTH % 4 != 0 || TTF_TEX_HEIGHT % 4 != 0) {
        LOG_MSG(error, "Assert fail, texture size isn't a multiple of block size!\n");
        return NULL;
    }

    u8* ttf_data = physfs_load_file(ttf_path);
    if (ttf_data == NULL) {
        return false;
    }

    stbtt_fontinfo font_info = {0};
    if (!stbtt_InitFont(&font_info, ttf_data, 0)) {
        free(ttf_data);
        return false;
    }

    u8 bitmap[TTF_TEX_HEIGHT][TTF_TEX_WIDTH] = {0};
    if (!stbtt_PackBegin(&pack_ctx, (unsigned char*)bitmap, TTF_TEX_WIDTH, TTF_TEX_HEIGHT, 0, 1, NULL)) {
        free(ttf_data);
        return false;
    }

    // Get font height data
    {
        int x0, x1, y0, y1;
        stbtt_GetFontBoundingBox(&font_info, &x0, &y0, &x1, &y1);
        font_height = y1 - y0;
        font_ascent = y1;
    }
    // stbtt_PackSetSkipMissingCodepoints(&state->pack_ctx, true);

    const float font_size = 64; // Characters will render up to 64 pixels high
    // If you add multiple ranges, you'll have to fix the index calculation in the transform update function!
    stbtt_PackFontRange(&pack_ctx, ttf_data, 0, font_size, FIRST_CHAR, NUM_CHAR, packed_chars);

    // After font packing, we can delete the TTF data
    free(ttf_data);

    // BC4 is 0.5 bytes per pixel
    u32 size = (TTF_TEX_HEIGHT * TTF_TEX_WIDTH) / 2;
    u8 bc4_bitmap[(TTF_TEX_HEIGHT * TTF_TEX_WIDTH) / 2] = {0};

    // Compress the texture
    u32 output_pos = 0;
    for (u16 i = 0; i < TTF_TEX_HEIGHT; i += 4) { // Rows
        for (u16 j = 0; j < TTF_TEX_WIDTH; j+= 4) { // Columns
            u8 bc4_in[16] = {0};
            // Copy the 4 rows of 4 pixels each into the input buffer
            for (u8 k = 0; k < 4; k++) {
                memcpy(&bc4_in[4 * k], &bitmap[i + k][j], 4);
            }

            // Compress the block
            stb_compress_bc4_block((u8*)bc4_bitmap + output_pos, bc4_in);
            output_pos += BC4_BLOCK_SIZE;
        }
    }

    texture out = {
        .channels = 1,
        .data = (u8*)bc4_bitmap,
        .width = TTF_TEX_WIDTH,
        .height = TTF_TEX_HEIGHT,
        .compressed = true,
        .fmt = BC4,
        .mip_level = 0,
    };
    img_write(out, "font_atlas.dds"); // For debugging

    // Upload font texture
    glGenTextures(1, &font_atlas);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font_atlas);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RED_RGTC1, TTF_TEX_WIDTH, TTF_TEX_HEIGHT, 0, size, bc4_bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Compile shaders
    char* vert_src = physfs_load_file("/src/editor/shader/text.vert");
    char* frag_src = physfs_load_file("/src/editor/shader/text.frag");
    if (vert_src == NULL || frag_src == NULL) {
        return NULL;
    }
    shader = program_compile_src(vert_src, frag_src);
    free(vert_src);
    free(frag_src);

    // Delete individual shader objects
    if (!shader_link_check(shader)) {
        LOG_MSG(error, "Shader linker failure!\n");
        stbtt_PackEnd(&pack_ctx);
        glDeleteTextures(1, &font_atlas);
        glDeleteProgram(shader);
        return NULL;
    }
    glUseProgram(shader); // Needed to set uniforms

    // Set our sampler to texture unit 0 for portability
    gl_obj atlas = glGetUniformLocation(shader, "font_atlas");
    u_transforms = glGetUniformLocation(shader, "transforms");
    u_texcoords = glGetUniformLocation(shader, "texcoords");
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

    initialized = true;
    return true;
}

void text_renderer_cleanup() {
    glDeleteProgram(shader);
    glDeleteTextures(1, &font_atlas);

    glDeleteVertexArrays(1, &tex_quad.vao);
    glDeleteBuffers(1, &tex_quad.vbuf);
    // tex quad shares the regular quad's index buffer, no need to delete it

    stbtt_PackEnd(&pack_ctx);
}

text_state text_render_prep(const char* text, u32 len, float scale, vec2 pos) {
    if (!initialized) {
        LOG_MSG(warning, "You haven't initialized the text renderer yet! I've silently ignored this, but nothing will render!\n");
        return (text_state){0};
    }

    // Copy arguments into struct.
    // We discard const here, but I pinky promise this is only so you can
    // change the pointer value to NULL later ;)
    // (allows you to free the string itself, but keep rendering it)
    text_state ctx = { .text = (char*)text, .scale = scale, .pos[0] = pos[0], .pos[1] = pos[1], };

    if (len > 0) {
        // Allows caller to override allocation size
        ctx.num_chars = len;
    }
    else {
        if (text == NULL) {
            LOG_MSG(error, "Caller gave a NULL string with no allocation hint\n");
            return ctx;
        }
        ctx.num_chars = strlen(text);
    }

    // Now that we know how many characters need drawing, we can allocate.

    // One transform per instanced character quad
    ctx.transforms = calloc(ctx.num_chars, sizeof(*ctx.transforms));
    if (ctx.transforms == NULL) {
        LOG_MSG(error, "Allocation failed for %d 4x4 matrices!\n", ctx.num_chars);
        return ctx;
    }
    // First 2 coords are top-left UVs, last 2 are the bottom-right UVs.
    ctx.texcoords = calloc(ctx.num_chars, sizeof(*ctx.texcoords));
    if (ctx.texcoords == NULL) {
        LOG_MSG(error, "Allocation failed for %d-element vec4 array!\n", ctx.num_chars);
        return ctx;
    }

    // The intuitive behaviour is for length to match the provided string
    if (len > 0) {
        if (text == NULL) {
            ctx.num_chars = 0;
            return ctx;
        }
        else {
            ctx.num_chars = strlen(text);
        }
    }
    
    if (text != NULL) {
        text_update_transforms(&ctx); // Make sure caller is ready to render
    }
    return ctx;
}

void text_update_transforms(text_state* ctx) {
    if (ctx->text == NULL) {
        LOG_MSG(warning, "Someone asked for a text update, but there's nothing to update to... [ignored]\n");
        return;
    }
    ctx->num_chars = strlen(ctx->text);

    // Get aspect ratio
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    const float aspect = (float)mode->width / (float)mode->height;
    // Scale characters 1/48th of the screen
    float scale_factor = (((float)mode->height / 32) / (float)font_height);

    const float ascent = (float)font_ascent / pack_ctx.height;


    const float scale = ctx->scale;
    float cur_x = ctx->pos[0];
    float cur_y = ctx->pos[1];
    u32 char_idx = 0;
    u8 char_len = 0;
    for (u32 i = 0; i < ctx->num_chars; i += char_len) {
        u32 codepoint = utf8_codepoint(&ctx->text[i], &char_len);
        if (FIRST_CHAR > codepoint || codepoint > (FIRST_CHAR + NUM_CHAR)) {
            // This is outside the Unicode range we cover, replace it w/
            // a control character that renders as a box.
            codepoint = 0x81;
        }
        // This math will break if we have multiple non-contiguous ranges!
        const u32 idx = codepoint - FIRST_CHAR;
        // Find out the character's texture coords & put them in the array
        stbtt_aligned_quad packed_quad = {0};
        mat4 model = {0};
        glm_mat4_identity(model);
        float width = 0;
        float height = 0;
        float start_height = 0;
        float x_advance = 0;
        {
            // This function requires a valid float output address, even though I don't need that output.
            float temp = 0;
            stbtt_GetPackedQuad(packed_chars, pack_ctx.width, pack_ctx.height, idx, &temp, &temp, &packed_quad, 0);

            x_advance = packed_chars[idx].xadvance * scale_factor;
            width = (packed_quad.x1 - packed_quad.x0) * scale_factor;
            height = ((packed_quad.y1 - packed_quad.y0) * scale_factor);
            // Figure out how much we need to go below the baseline. Frankly I'm
            // not sure why multiplying by 2 is needed, but it looks wrong if we
            // don't do it. We should look into this math...
            start_height = -(packed_quad.y1 * scale_factor * 2);
        }

        // Since quad origin is the middle, first character needs to advance
        // by half the quad width so its left edge matches the intended position.
        // At some point we should adjust the quad origin and fix this.
        if (i == 0) {
            x_advance /= 2;
        }
        // Advance on X by character width
        cur_x += x_advance * scale;

        // Apply our transforms for this character
        vec3 pos_diff = {
            cur_x,
            // Origin is top-left, so we go down by ascent to be at baseline.
            // Then go down by the amount of empty space between the line top
            // and character top, and go back up by the offset from baseline.
            cur_y - ((ascent + (ascent - height) - start_height) * scale),
            0, // No depth needed
        };
        glm_translate(model, pos_diff);
        glm_scale(model, (vec3){scale * width, scale * aspect * height, scale});
        glm_rotate_x(model, glm_rad(90.0f), model); // Face quad towards camera
        memcpy(&ctx->transforms[char_idx], model, sizeof(model));

        ctx->texcoords[char_idx][0] = packed_quad.s0; // Upper-left texcoord
        ctx->texcoords[char_idx][1] = packed_quad.t0;
        ctx->texcoords[char_idx][2] = packed_quad.s1; // Bottom-right texcoord
        ctx->texcoords[char_idx][3] = packed_quad.t1;

        char_idx++;
    }
}

void text_render(text_state ctx) {
    if (ctx.transforms == NULL || ctx.texcoords == NULL) {
        return; // Avoid segfaults
    }
    // Upload transforms & render
    glUseProgram(shader);
    glBindVertexArray(tex_quad.vao);
    glUniformMatrix4fv(u_transforms, ctx.num_chars, GL_FALSE, (const float*)ctx.transforms);
    glUniform4fv(u_texcoords, ctx.num_chars, (const float*)ctx.texcoords);
    glDrawElementsInstanced(GL_TRIANGLES, tex_quad.idx_count, GL_UNSIGNED_SHORT, NULL, ctx.num_chars);

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void text_free(text_state ctx) {
    free(ctx.transforms);
    free(ctx.texcoords);
}

