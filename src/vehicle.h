#ifndef VEHICLE_H
#define VEHICLE_H
#include <stdio.h>

#include "common/int.h"
#include "common/vector.h"
#include "common/logging.h"

typedef struct {
    u64 magic; // VEHICLE_MAGIC
    u16 part_count;
    u8 is_one_piece;
    u8 unk[0x1D]; // This is very incomplete, there's lots of other data here.
    c16 name[0x20];
    u8 unk2[0x18];
    // If you add new fields, make sure to update vehicle_header_byteswap().
}vehicle_header;

enum {
    VEHICLE_MAGIC = 0x3F9AE14840547AE1,
};

typedef struct {
    u32 unknown;
    vec3u8 pos;
    u16 pad;
    u8 modifier; // Used for part settings, if applicable (e.g. wheel steering mode)
    union {
        u8 pad2;
        bool selected; // Used only in this editor, set to 0 before writing
    };
    u32 id; // part_id enum
    vec3 rot; // Measured in radians
    rgba8 color; // Game stores arbitrary colors, but only the preset colors are allowed
    u32 pad3;
}part_entry;

// The flexible array member causes sizeof(vehicle) == sizeof(vehicle_header)!
// Be careful!
typedef struct {
    vehicle_header head;
    part_entry parts[];
}vehicle;

enum {
    PART_SCALE = 1,
    PART_POS_SCALE = 2,
    SEL_BOX_SIZE = (PART_POS_SCALE * 2),

    // Used to store a compact 3D grid of parts, with 1 bit per cell indicating
    // if the cell is already used. Part coordinates are a u8, so at 1 bit per
    // cell the bitmask is 32 bytes wide.
    VEH_MAX_DIM = ((UINT8_MAX + 1) / 8)
};

// 3D array to store parts at 1 bit each
typedef u8 vehicle_bitmask[UINT8_MAX + 1][UINT8_MAX + 1][VEH_MAX_DIM];


// Load a vehicle into a newly allocated buffer. Automatically handles STFS if
// needed (it's assumed that the vehicle is stored as the first file entry, and
// is under 680KiB).
// Returns NULL on failure, caller must free the buffer.
vehicle* vehicle_load(const char* path);

// Uses part data to find the centerpoint of a vehicle.
// (returns float vector for convenience)
vec3s vehicle_find_center(vehicle* v);

// Unselect all parts in a vehicle
void vehicle_unselect_all(vehicle* v);

// Move a part by a 3D vector. If the new position is out of bounds (< 0), that
// position will be the new zero and the other parts are adjusted accordingly.
// Returns a boolean indicating if the vehicle had to be adjusted.
bool vehicle_move_part(vehicle* v, u16 idx, vec3s16 diff);

// Check if the selected parts overlap with the bitmask of unselected parts.
bool vehicle_selection_overlap(vehicle* v, vehicle_bitmask* mask);

void update_vehiclemask(vehicle* v, vehicle_bitmask* mask);

// Look up a part by its position.
// Returns NULL if no part is found, so always check the result!
part_entry* part_by_pos(vehicle* v, vec3u8 pos);


// NOTE: See common/endian.h for details on endian-ness.

// Byte-swap an existing part entry
void part_byteswap(part_entry* part);

// Byte-swap an existing vehicle header
void vehicle_header_byteswap(vehicle_header* v);

#endif // VEHICLE_H
