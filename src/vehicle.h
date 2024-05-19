#ifndef VEHICLE_H
#define VEHICLE_H
#include <stdio.h>

#include "common/int.h"
#include "common/vector.h"
#include "common/logging.h"

typedef struct {
    u64 magic; // == VEHICLE_MAGIC
    u16 part_count;
    u8 is_one_piece;
    u8 pad;
    // This struct is very incomplete, there's lots of other data here.
}vehicle_header;

enum {
    PART_ENTRIES_OFFSET = 0x80,
    VEHICLE_MAGIC = 0x3F9AE14840547AE1,
};

typedef struct {
    u32 unknown;
    vec3u8 pos;
    u16 pad;
    u8 modifier; // Used for part settings, if applicable (e.g. wheel steering mode)
    u8 pad2;
    u32 id; // part_id enum
    vec3f rot; // Measured in radians
    rgba8 color; // Game stores arbitrary colors, but only the preset colors are allowed
    u32 pad3;
}part_entry;

static void check_part_entry() {
    if (sizeof(part_entry) != 0x24) {
        LOG_MSG(error, "sizeof(part_entry) == 0x%x, expected 0x24\n", sizeof(part_entry));
    }
}

// Automatically handles byte-swapping the big-endian fields
part_entry part_read(FILE* f);

// Byte-swap an existing part entry
void part_byteswap(part_entry* part);

void vehicle_header_byteswap(vehicle_header* v);

#endif // VEHICLE_H
