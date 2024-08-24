#ifndef VEHICLE_H
#define VEHICLE_H
#include <assert.h>
#include "common/int.h"
#include "common/vector.h"
// Vehicle data structures for Nuts & Bolts. The header has a lot of holes, but
// the part entries are mostly complete.

typedef struct {
    u64 magic; // VEHICLE_MAGIC
    u16 part_count;
    u8 is_one_piece;
    u8 unk;
    float unknownf; // Usually an exact integer?
    u8 unk1[8];
    float weight;
    float unknownf_1;
    u8 unk2[0x8]; // This is very incomplete, there's lots of other data here.
    c16 name[0x20];
    u8 unk3[0x18];
    // If you add new fields, make sure to update vehicle_header_byteswap().
}vehicle_header;
static_assert(sizeof(vehicle_header) == 0x80, "vehicle_header size is wrong!");

enum {
    // No idea why they chose these numbers or if they mean anything.
    VEHICLE_MAGIC = 0x3F9AE14840547AE1,
};

// Just a heads-up: The "padding" in here is probably not actually padding.
typedef struct {
    u32 unknown;
    vec3s8 pos;
    u16 pad;
    u8 modifier; // Used for part settings, if applicable (e.g. wheel steering mode)
    u8 pad2;
    u32 id; // part_id enum
    vec3 rot; // 3D Euler rotation in radians. The preview will render w/ arbitrary angles, but upon loading rounds to 90 degrees
    rgba8 color; // Game stores arbitrary colors, but only the preset colors are allowed
    u32 pad3;
}part_entry;
static_assert(sizeof(part_entry) == 0x24, "part_entry size is wrong!");

// The flexible array member causes sizeof(vehicle) == sizeof(vehicle_header)!
// Be careful!
typedef struct {
    vehicle_header head;
    part_entry parts[];
}vehicle;

// Load a vehicle into a newly allocated buffer. Automatically handles STFS if
// needed (it's assumed that the vehicle is stored as the first file entry, and
// is under 680KiB).
// Returns NULL on failure, caller must free the buffer.
vehicle* vehicle_load(const char* path);

// NOTE: See common/endian.h for details on endian-ness.

// Byte-swap an existing part entry
void part_byteswap(part_entry* part);

// Byte-swap an existing vehicle header
void vehicle_header_byteswap(vehicle_header* v);

// Uses part data to find the centerpoint of a vehicle.
// (returns float vector for convenience, since centerpoint could be a decimal)
vec3s vehicle_find_center(vehicle* v);

// Everything after this point involves runtime data structures for the editor.
// If you're using this header to write your own program, the rest won't be
// useful unless you adopt the same data structures.

enum {
    PART_SCALE = 1,
    PART_POS_SCALE = 2, // Coordinate multiplier for rendering (can add spacing in the part grid)
    SEL_BOX_SIZE = (PART_POS_SCALE * 2),

    VEH_MAX_DIM = (INT8_MAX + 1),
    VEH_MASK_BYTE_WIDTH = (VEH_MAX_DIM / 8),
};

// Unselect all parts in a vehicle
void vehicle_unselect_all(vehicle* v);

// Move a part by a 3D vector. If the new position is out of bounds (< 0), that
// position will be the new zero and the other parts are adjusted accordingly.
// If the vehicle was adjusted, writes to an output vector to indicate the
// vector of the adjustment. This output vector can be NULL.
// Returns a boolean indicating if the vehicle had to be adjusted.
bool vehicle_move_part(vehicle* v, u16 idx, vec3s16 diff, vec3s16* adjust_out);
#endif // VEHICLE_H

