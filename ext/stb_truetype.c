// Compile this file for STB's TrueType implementation

// Adding rect pack makes the TrueType rasterizer use its way better packing algo
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
