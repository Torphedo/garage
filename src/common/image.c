#include <stdio.h>
#include <stdbool.h>
#include <memory.h>

#include "int.h"
#include "image.h"
#include "file.h"
#include "logging.h"

// 32MiB image data buffer for OpenGL to copy from.
u8 img_buf[32 * 0x400 * 0x400] = {0};

typedef enum dds_flags {
    DDSD_CAPS        = 1 << 0,
    DDSD_HEIGHT      = 1 << 1,
    DDSD_WIDTH       = 1 << 2,
    DDSD_PITCH       = 1 << 3,
    DDSD_PIXELFORMAT = 1 << 12,
    DDSD_MIPMAPCOUNT = 1 << 17,
    DDSD_LINEARSIZE  = 1 << 19,
    DDSD_DEPTH       = 1 << 23,

    REQUIRED_BASE_FLAGS = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
}dds_flags;

typedef enum dds_caps_flags {
    DDSCAPS_COMPLEX = 0x00000008,
    DDSCAPS_MIPMAP  = 0x00400000,
    DDSCAPS_TEXTURE = 0x00001000
}dds_caps_flags;

typedef enum dds_format_flags {
    DDPF_ALPHAPIXELS = 0x00000001,
    DDPF_ALPHA       = 0x00000002,
    DDPF_FOURCC      = 0x00000004,
    DDPF_RGB         = 0x00000040,
    DDPF_YUV         = 0x00000200,
    DDPF_LUMINANCE   = 0x00020000
}dds_format_flags;

typedef enum {
    DDS_DXT1 = 0x31545844, // 'DXT1' (little endian)
    DDS_DXT3 = 0x33545844, // 'DXT3' (little endian)
    DDS_DXT5 = 0x35545844, // 'DXT5' (little endian)
    DDS_DX10 = 0x30315844, // 'DX10' (little endian)
    DXT1_BLOCK_SIZE = 0x8,
    DXT3_BLOCK_SIZE = 0x10,
    DXT5_BLOCK_SIZE = 0x10
}dds_bc_format;


typedef struct dds_pixel_format {
    u32 size; // Must be 32 (0x20)
    u32 flags;
    u32 format_char_code; // See dwFourCC here: https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat
    u32 bits_per_pixel;
    u32 red_bitmask;
    u32 green_bitmask;
    u32 blue_bitmask;
    u32 alpha_bitmask;
}dds_pixel_format;

// We can't just check equality with "DDS" because it's 'DDS ' with no null
// terminator.
typedef enum {
    DDS_BEGIN = 0x20534444 // 'DDS ' (little endian)
}dds_const;

typedef struct dds_header {
    u32 identifier;   // DDS_BEGIN as defined above. aka "file magic" / "magic number".
    u32 size;         // Must be 124 (0x7C)
    u32 flags;
    u32 height;
    u32 width;
    u32 pitch_or_linear_size;
    u32 depth;
    u32 mipmap_count;
    u32 reserved[11]; // Unused
    dds_pixel_format pixel_format;
    u32 caps;         // Flags for complexity of the surface
    u32 caps2;        // Always 0 because we don't use cubemaps or volumes
    u32 caps3;        // Unused
    u32 caps4;        // Unused
    u32 reserved2;    // Unused
}dds_header;

dds_header mk_header(u32 height, u32 width, u32 mip_lvl) {
    dds_header header = {
        .identifier = DDS_BEGIN,
        .size = 0x7C,
        .height = height,
        .width = width,
        .depth = 0,
        .mipmap_count = mip_lvl,
        .flags = REQUIRED_BASE_FLAGS | (DDSD_MIPMAPCOUNT * (mip_lvl > 0)),
        .pixel_format = {
                .size = sizeof(dds_pixel_format)
        },
        // Only enable mipmap flags when mip level > 0
        .caps = DDSCAPS_TEXTURE | ((DDSCAPS_MIPMAP | DDSCAPS_COMPLEX) * (mip_lvl > 0)),
    };
    return header;
}

void img_snap(texture* img, u32 size) {
    if (img->width % size) {
        img->width -= (img->width % size);
    }
    if (img->height % size) {
        img->height -= (img->height % size);
    }
}

bool has_flag(u32 input, u32 flag) {
    return (input ^ flag) != input;
}

u32 dxt_pitch(u32 height, u32 width, u32 block_size) {
    u32 block_res = 16; // 16 pixels per block
    // TODO: Divide by pixels per byte instead, so we don't need to use floats.
    float bytes_per_pixel = (float)block_size/ (float)block_res;
    u32 pitch = (width * height) * bytes_per_pixel;
    return pitch;
}


void img_write(texture img, const char* path) {
    u32 tex_size = 0;

    dds_header header = mk_header(img.height, img.width, img.mip_level);
    if (img.compressed) {
        header.flags |= DDSD_LINEARSIZE;
        u8 block_size = DXT1_BLOCK_SIZE;

        // Handle each format
        switch (img.fmt) {
            case DXT1:
                header.pixel_format.format_char_code = DDS_DXT1;
                break;
            case DXT3:
                header.pixel_format.format_char_code = DDS_DXT3;
                block_size = DXT3_BLOCK_SIZE;
                break;
            case BC4:
                block_size = DXT1_BLOCK_SIZE;
                header.pixel_format.format_char_code = MAGIC('A', 'T', 'I', '1');
                break;
            default:
                header.pixel_format.format_char_code = DDS_DXT5;
                block_size = DXT5_BLOCK_SIZE;
                break;
        }

        header.pitch_or_linear_size = dxt_pitch(img.height, img.width, block_size);
        header.pixel_format.flags = DDPF_FOURCC;

        // tex_size = header.pitch_or_linear_size + ((img.mip_level - 1) * (header.pitch_or_linear_size * 0.333333f));
        tex_size = header.pitch_or_linear_size;
    }
    else {
        header.flags |= DDSD_PITCH;
        u32 bytes_per_channel = 8 << img.unit_size;
        header.pitch_or_linear_size = bytes_per_channel * (img.channels) * img.width;
        header.pixel_format.bits_per_pixel = bytes_per_channel * img.channels;
        tex_size = header.pitch_or_linear_size * img.height;

        header.pixel_format.flags |= DDPF_RGB;
        if (img.channels > 4) {
            header.pixel_format.flags |= DDPF_ALPHAPIXELS;
        }
        // if (img.channels == 2) {
        //     header.pixel_format.flags = DDPF_ALPHAPIXELS | DDPF_LUMINANCE;
        // }

        if (img.unit_size == 1) {
            // 16-bit
            header.pixel_format.red_bitmask = 0xFFFF;
            if (img.channels > 1) {
                header.pixel_format.green_bitmask = 0xFFFF << 16;
            }
        }
        else {
            switch (img.channels) {
                case 4:
                    header.pixel_format.alpha_bitmask = 0xFF << 24;
                case 3:
                    header.pixel_format.blue_bitmask = 0xFF << 16;
                case 2:
                    header.pixel_format.green_bitmask = 0xFF << 8;
                case 1:
                    header.pixel_format.red_bitmask = 0xFF;
            }
        }
    }

    FILE* out = fopen(path, "wb");
    if (out == NULL) {
        return;
    }
    fwrite(&header, sizeof(header), 1, out);
    fwrite(img.data, tex_size, 1, out);
    fclose(out);

    LOG_MSG(info, "Written to %s\n", path);
}

bool is_dds(char* filename) {
    if (!file_exists(filename)) {
        return false;
    }
    FILE* f = fopen(filename, "rb");
    if (f == NULL) {
        return false;
    }
    u32 magic = 0;
    fread(&magic, sizeof(magic), 1, f);
    fclose(f);

    return (magic == DDS_BEGIN);
}

texture image_buf_load(char* filename) {
    texture img = {
        .data = img_buf,
        .width = 512,
        .height = 512,
        .mip_level = 1,
        .fmt = DXT1,
        .compressed = true,
        .channels = 4,
        .unit_size = 0
    };
    if (!file_exists(filename)) {
        return img;
    }

    // 0xCC bytes separate pixels with missing data from black pixels
    memset(img.data, 0xCC, sizeof(img_buf));
    if (!is_dds(filename)) {
        // Load raw image data
        file_load_existing(filename, img.data, sizeof(img_buf));
        return img;
    }

    u32 size = file_size(filename);
    FILE* f = fopen(filename, "rb");
    if (f == NULL) {
        return img;
    }
    dds_header header = {0};
    fread(&header, sizeof(header), 1, f);

    // Read in remaining image data from the DDS
    fread(img.data, size - sizeof(header), 1, f);
    fclose(f);

    // Use data from the DDS as our initial texture state
    img.width = header.width;
    img.height = header.height;

    // Only inherit the mip count if the flag in the header is set
    bool has_mipmapcount = ((header.flags & DDSD_MIPMAPCOUNT) != 0);
    img.mip_level = header.mipmap_count * has_mipmapcount;

    // Presence of FOURCC flag indicates a compressed texture format
    img.compressed = ((header.pixel_format.flags & DDPF_FOURCC) != 0);
    u32 dxt_n = header.pixel_format.format_char_code;
    switch (dxt_n) {
        case DDS_DXT5:
            img.fmt = DXT3;
            break;
        case DDS_DXT3:
            img.fmt = DXT5;
            break;
        default:
            img.fmt = DXT1;
            break;
    };
     
    // TODO: set channel count for uncompressed textures
    return img;
}
