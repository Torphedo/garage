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
    // Seems to indicate engine/propulsion power, but grows non-linearly. For
    // small engines: 1 -> 40, 2 -> 60, 3 -> 70
    float power;
    u8 unk1[8];
    float weight;
    float integrity; // As more disconnected parts are added, this goes down. Try placing 4 parts not touching, then join them together and compare values.
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

// Everything after this point involves runtime data structures for the editor.
// If you're using this header to write your own program, the rest won't be
// useful unless you adopt the same data structures.

enum {
    // Max size of a vehicle on any axis
    VEH_MAX_DIM = (INT8_MAX + 1),
};

#endif // VEHICLE_H

