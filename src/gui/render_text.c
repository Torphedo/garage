#include <stddef.h>
#include <stdio.h>
#include <memory.h>
#include <malloc.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include <stb_truetype.h>
#define STB_DXT_IMPLEMENTATION
#include <stb_dxt.h>

#include <common/file.h>
#include <common/logging.h>
#include <common/image.h>
#include "gui_common.h"

enum {
    TTF_BUF_SIZE = 1 << 20,
    TTF_TEX_WIDTH = 512,
    TTF_TEX_HEIGHT = 1024,
    BC4_BLOCK_SIZE = 8,
};

u8* ttf_to_bitmap(const char* ttf_path) {
    if (TTF_TEX_WIDTH % 4 != 0 || TTF_TEX_HEIGHT % 4 != 0) {
        LOG_MSG(error, "Assert fail, texture size isn't a multiple of block size!\n");
        return NULL;
    }

    const u32 num_char = 256;
    const u32 first_char = 32;
    stbtt_bakedchar cdata[num_char];

    u8* ttf_buffer = file_load(ttf_path);
    u8 bitmap[TTF_TEX_HEIGHT][TTF_TEX_WIDTH] = {0};
    if (ttf_buffer == NULL) {
        LOG_MSG(error, "Failed to load file\n");
        return NULL;
    }
    // float char_height = ((TTF_TEX_WIDTH / 8) + (TTF_TEX_HEIGHT / 8)) / 2;
    float char_height = (TTF_TEX_WIDTH / 8);
    stbtt_BakeFontBitmap(ttf_buffer, 0, char_height, (unsigned char*)bitmap, TTF_TEX_WIDTH, TTF_TEX_HEIGHT, first_char, num_char, cdata); // no guarantee this fits!
    free(ttf_buffer);

    u8* out = calloc(1, (TTF_TEX_WIDTH * TTF_TEX_HEIGHT) / 2);
    u32 output_pos = 0;
    for (u16 i = 0; i < TTF_TEX_HEIGHT; i += 4) { // Rows
        for (u16 j = 0; j < TTF_TEX_WIDTH; j+= 4) { // Columns
            u8 bc4_in[16] = {0};
            // Copy the 4 rows of 4 pixels each into the input buffer
            for (u8 k = 0; k < 4; k++) {
                memcpy(&bc4_in[4 * k], &bitmap[i + k][j], 4);
            }

            // Compress the block
            stb_compress_bc4_block(out + output_pos, bc4_in);
            output_pos += BC4_BLOCK_SIZE;
        }
    }

    return out;
}

const char* ttf_path = "ProFontIIxNerdFontMono-Regular.ttf";

typedef struct {
    gui_state* gui;
    u8* bc4_bitmap;
}text_state;


void* text_init(gui_state* gui) {
    text_state* state = calloc(1, sizeof(*state));
    if (state == NULL) {
        return NULL;
    }

    state->bc4_bitmap = ttf_to_bitmap(ttf_path);
    if (state->bc4_bitmap == NULL) {
        LOG_MSG(error, "Failed to render bitmap\n");
        return NULL;
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
    img_write(out, "out.dds");

    return state;
}

void text_render(text_state* ctx) {

}

void text_destroy(text_state* ctx) {
    free(ctx->bc4_bitmap);
    free(ctx);
}

typedef void (*renderproc)(void*);
renderer text = {
    text_init,
    (renderproc)text_render,
    (renderproc)text_destroy,
};

