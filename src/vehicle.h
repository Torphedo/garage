#ifndef VEHICLE_H
#define VEHICLE_H

#include "common/int.h"
#include "common/vector.h"

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

enum {
    VEHICLE_MAGIC = 0x3F9AE14840547AE1,
};

typedef struct {
    u32 unknown;
    vec3s8 pos;
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

// Look up a part by its origin position.
// This will never return NULL, if a part isn't found the struct will be all 0.
part_entry* part_by_pos(vehicle* v, vec3s8 target);

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

// Used to store a compact 3D grid of parts at 1 bit per cell.
// The editor uses 2 of these, one for storing which cells are occupied and one
// for storing which cells are selected.
typedef u8 vehicle_bitmask[VEH_MAX_DIM][VEH_MAX_DIM][VEH_MASK_BYTE_WIDTH];

// Safely get & set values from vehicle bitmask (with bounds checking)
bool vehiclemask_get_3d(vehicle_bitmask* mask, s8 x, s8 y, s8 z);
void vehiclemask_set_3d(vehicle_bitmask* mask, s8 x, s8 y, s8 z, u8 val);

// Unselect all parts in a vehicle
void vehicle_unselect_all(vehicle* v);

// Move a part by a 3D vector. If the new position is out of bounds (< 0), that
// position will be the new zero and the other parts are adjusted accordingly.
// Returns a boolean indicating if the vehicle had to be adjusted.
bool vehicle_move_part(vehicle* v, u16 idx, vec3s16 diff);

// Check if the selected parts overlap with the rest of the vehicle
bool vehicle_selection_overlap(vehicle* v, vehicle_bitmask* mask);

void update_vehiclemask(vehicle* v, vehicle_bitmask* vacancy, vehicle_bitmask* selection);

#endif // VEHICLE_H
