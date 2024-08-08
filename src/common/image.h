#ifndef IMAGE_H
#define IMAGE_H
#include <stdbool.h>
#include "int.h"

typedef enum {
    DXT1, // BC1
    DXT3, // BC2
    DXT5, // BC3
    BC4,
    DXT_ENUM_MAX,
}img_fmt_compressed;

enum {
    // Width/height (in pixels) of a compressed texture block
    COMPRESSED_BLK_DIM = 4,
};

typedef struct {
    u8* data;
    u16 width; // u16 is plenty for any image
    u16 height;
    u16 mip_level; // And DEFINITELY for mips...

    bool compressed;
    img_fmt_compressed fmt; // Compressed format

    // Only used if uncompressed
    u8 unit_size; // 0 = u8, 1 = u16, 2 = u32
    u8 channels;
}texture;

// Round image dimensions down to some value
void img_snap(texture* img, u32 size);

// Save an image to a DDS file
void img_write(texture img, const char* path);

// Load a DDS from disk
// TODO: Make this take a DDS buffer instead of a filename?
texture image_buf_load(char* filename);

#endif // #ifndef IMAGE_H

