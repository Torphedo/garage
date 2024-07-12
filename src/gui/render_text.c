#include <stddef.h>
#include <memory.h>
#include <malloc.h>
#include <string.h>

#include <glad/glad.h>
#include <stb_truetype.h>
#include <stb_dxt.h>

#include <common/file.h>
#include <common/logging.h>
#include <common/image.h>
#include <common/utf8.h>
#include "shader.h"
#include "primitives.h"
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
    TEXT_QUAD_SIZE = 1,

    // The lower bound and size of the Unicode range we support.
    // Unicode 0x20 - 0xFF should cover English, German, Spanish, etc.
    FIRST_CHAR = 0x20,
    NUM_CHAR = 0xDF,
};

// Regular quad, but with texture coordinates instead of colors
const tex_vertex texquad_vertices[] = {
    {
        .position = {TEXT_QUAD_SIZE, -1.5f, TEXT_QUAD_SIZE},
        .texcoord = {1.0f, 1.0f},
    },
    {
        .position = {TEXT_QUAD_SIZE, -1.5f, -TEXT_QUAD_SIZE},
        .texcoord = {1.0f, 0},
    },
    {
        .position = {-TEXT_QUAD_SIZE, -1.5f, -TEXT_QUAD_SIZE},
        .texcoord = {0, 0},
    },
    {
        .position = {-TEXT_QUAD_SIZE,  -1.5f, TEXT_QUAD_SIZE},
        .texcoord = {0, 1.0f},
    }
};

model tex_quad = {
    .vert_count = ARRAY_SIZE(texquad_vertices),
    .idx_count = ARRAY_SIZE(quad_indices),
    .vertices = texquad_vertices,
    .indices = quad_indices,
};

const char vert_src[] = {
#include "shader/text.vert.h"
};

const char frag_src[] = {
#include "shader/text.frag.h"
};

stbtt_packedchar packed_chars[NUM_CHAR];
u8* ttf_data;
stbtt_fontinfo font_info;
stbtt_pack_context pack_ctx;

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

    ttf_data = file_load(ttf_path);
    u8 bitmap[TTF_TEX_HEIGHT][TTF_TEX_WIDTH] = {0};
    if (ttf_data == NULL) {
        LOG_MSG(error, "Failed to load TTF!\n");
        free(ttf_data);
        return false;
    }
    stbtt_InitFont(&font_info, ttf_data, 0);
    stbtt_PackBegin(&pack_ctx, (unsigned char*)bitmap, TTF_TEX_WIDTH, TTF_TEX_HEIGHT, 0, 1, NULL);
    // stbtt_PackSetSkipMissingCodepoints(&state->pack_ctx, true);
    float font_size = ((float)TTF_TEX_WIDTH / 8);
    stbtt_PackFontRange(&pack_ctx, ttf_data, 0, font_size, FIRST_CHAR, NUM_CHAR, packed_chars);

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Compile shaders
    gl_obj vert_shader = shader_compile_src(vert_src, GL_VERTEX_SHADER);
    gl_obj frag_shader = shader_compile_src(frag_src, GL_FRAGMENT_SHADER);
    if (vert_shader == 0 || frag_shader == 0) {
        LOG_MSG(error, "Shader compilation failure!\n");
        stbtt_PackEnd(&pack_ctx);
        free(ttf_data);
        glDeleteTextures(1, &font_atlas);
        return NULL;
    }

    // Create & link final shader program
    shader = glCreateProgram();
    glAttachShader(shader, vert_shader);
    glAttachShader(shader, frag_shader);
    glLinkProgram(shader);

    // Delete individual shader objects
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    if (!shader_link_check(shader)) {
        LOG_MSG(error, "Shader linker failure!\n");
        stbtt_PackEnd(&pack_ctx);
        free(ttf_data);
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

    return true;
}

void text_renderer_cleanup() {
    glDeleteProgram(shader);
    glDeleteTextures(1, &font_atlas);

    glDeleteVertexArrays(1, &tex_quad.vao);
    glDeleteBuffers(1, &tex_quad.vbuf);
    // tex quad shares the regular quad's index buffer, no need to delete it

    stbtt_PackEnd(&pack_ctx);
    free(ttf_data);
}

text_state text_render_prep(const char* text, u32 len) {
    // We discard const here, but I pinky promise this is only so you can
    // change the pointer value to NULL ;)
    // (allows you to free the string itself, but keep rendering it)
    text_state ctx = { .text = (char*)text };

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

    const float scale = 0.03f;
    float cur_x = -1.0f + (TEXT_QUAD_SIZE * scale);
    float cur_y = 0;
    u32 char_idx = 0;
    u8 char_len = 0;
    for (u32 i = 0; i < ctx->num_chars; i += char_len) {
        u32 codepoint = utf8_codepoint(&ctx->text[i], &char_len);
        if (FIRST_CHAR > codepoint || codepoint > (FIRST_CHAR + NUM_CHAR)) {
            // This is outside the Unicode range we cover, replace it w/
            // a control character that renders as a box.
            codepoint = 0x81;
        }
        const u32 idx = codepoint - FIRST_CHAR;
        // Find out the character's texture coords & put them in the array
        stbtt_aligned_quad packed_quad = {0};
        {
            // This function requires a valid float output address, even though I don't need that output.
            float temp;
            stbtt_GetPackedQuad(packed_chars, pack_ctx.width, pack_ctx.height, idx, &temp, &temp, &packed_quad, 0);
        }
        mat4 model = {0};
        glm_mat4_identity(model);
        float left_bearing = 0;
        float width = 0;
        float height = 0;
        float start_height = 0;
        {
            // Get left bearing
            int i_width, i_left; // The "i" is for "integer".
            stbtt_GetCodepointHMetrics(&font_info, codepoint, &i_width, &i_left);
            left_bearing = (float)i_left / pack_ctx.width;

            // Get width/height
            int x0, y0, x1, y1;
            stbtt_GetCodepointBox(&font_info, codepoint, &x0, &y0, &x1, &y1);
            width = (float)(x1 - x0) / pack_ctx.width;
            height = (float)(y1 - y0) / pack_ctx.height;
            start_height = (float)y0 / pack_ctx.height; // Offset from baseline
        }

        float delta_x = width;
        // Scoots thin characters back by the appropriate amount
        cur_x -= left_bearing * scale * 2;

        // LOG_MSG(debug, "%c is %f wide, %f left bearing, ", text[i], width, left_bearing);
        // printf("delta x = %f, cur_x = %f\n", delta_x, cur_x);

        // Apply our transforms for this character
        glm_translate(model, (vec3){cur_x, 0.7f - ((1.0f - height - (start_height * 2)) * 2 * scale), 0.2f});
        glm_scale(model, (vec3){scale * delta_x, scale * aspect * height, scale});
        glm_rotate_x(model, glm_rad(90.0f), model); // Face quad towards camera
        memcpy(&ctx->transforms[char_idx], model, sizeof(model));

        ctx->texcoords[char_idx][0] = packed_quad.s0; // Upper-left texcoord
        ctx->texcoords[char_idx][1] = packed_quad.t0;
        ctx->texcoords[char_idx][2] = packed_quad.s1; // Bottom-right texcoord
        ctx->texcoords[char_idx][3] = packed_quad.t1;
        char_idx++;

        // Advance on X by character width (* 2 because that's distance from center)
        cur_x += 2 * (delta_x + left_bearing + 0.2f) * scale;
    }
}

void text_render(text_state ctx) {
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

