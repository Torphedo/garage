#include <stdbool.h>
#include <malloc.h>

#include "common/endian.h"
#include "common/file.h"

#include "vehicle.h"
#include "common/logging.h"
#include "stfs.h"

// TODO: Make this a no-op when compiling for a big endian system
void part_byteswap(part_entry* part) {
    ENDIAN_FLIP(u32, part->unknown);
    ENDIAN_FLIP(u32, part->id);
    vec3_byteswap(&part->rot);

    // Byte-swap the padding just in case :P
    ENDIAN_FLIP(u16, part->pad);
    ENDIAN_FLIP(u32, part->pad3);
}

void vehicle_header_byteswap(vehicle_header* v) {
    ENDIAN_FLIP(u64, v->magic);
    ENDIAN_FLIP(u16, v->part_count);
    for (u32 i = 0; i < 0x20; i++) {
        ENDIAN_FLIP(c16, v->name[i]);
    }
}

vec3s vehicle_find_center(vehicle* v) {
    vec3u8 max = {0}; // Max XYZ position found in the vehicle
    for (u16 i = 0; i < v->head.part_count; i++) {
        vec3u8 pos = v->parts[i].pos;

        // Update the max position if the part's position is larger on any axis
        max = (vec3u8) {
            MAX(max.x, pos.x),
            MAX(max.y, pos.y),
            MAX(max.z, pos.z),
        };
    }

    // Return the centerpoint. Since all coordinates are u8, we don't need to
    // find the min point (it's just [0, 0, 0], we can divide by 2 instead).
    return (vec3s) {
        .x = (float)max.x / 2,
        .y = (float)max.y / 2,
        .z = (float)max.z / 2,
    };
}

vehicle* vehicle_load(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        LOG_MSG(error, "Failed to open %s\n", path);
        return NULL;
    }

    vehicle_header head = {0};
    fread(&head, sizeof(head), 1, f);
    fclose(f);
    vehicle_header_byteswap(&head);

    vehicle* v = NULL;
    if (head.magic == VEHICLE_MAGIC) {
        // Header claims it's a vehicle file, just load it directly
        LOG_MSG(debug, "Loading vehicle from raw save file %s\n", path);
        v = (vehicle*)file_load(path); // Reads file into a buffer for us
    }
    else {
        // Not a vehicle file, try to load it as an STFS save file.
        LOG_MSG(debug, "Loading vehicle from STFS save entry\n");
        v = (vehicle*)stfs_get_vehicle(path);
    }

    if (v == NULL) {
        LOG_MSG(error, "Failed to load vehicle\n");
        return NULL;
    }

    // Byte-swap everything & return result
    vehicle_header_byteswap(&v->head);
    for (u16 i = 0; i < v->head.part_count; i++) {
        part_byteswap(&v->parts[i]);
    }
    return v;
}

void vehicle_unselect_all(vehicle* v) {
    for (u16 i = 0; i < v->head.part_count; i++) {
        v->parts[i].selected = false;
    }
}

bool vehicle_part_conflict(vehicle_bitmask* mask, part_entry* p) {
    vec3u8 pos = p->pos;
    u8* addr = (u8*)&(*mask)[pos.x][pos.y];
    return mask_get(addr, pos.z);
}

bool vehicle_selection_overlap(vehicle* v, vehicle_bitmask* mask) {
    for (u16 i = 0; i < v->head.part_count; i++) {
        if (!v->parts[i].selected) {
            continue;
        }
        if (vehicle_part_conflict(mask, &v->parts[i])) {
            return true;
        }
    }
    return false;
}

void update_vehiclemask(vehicle* v, vehicle_bitmask* mask) {
    // Erase the bitmask
    memset(mask, 0x00, sizeof(vehicle_bitmask));

    for (u16 i = 0; i < v->head.part_count; i++) {
        vec3u8 pos = v->parts[i].pos;
        u8* addr = (u8*)&(*mask)[pos.x][pos.y];
        // Set the bit unless the part is selected
        mask_set(addr, pos.z, 1 * !v->parts[i].selected);
    }
}

bool vehicle_move_part(vehicle* v, u16 idx, vec3s16 diff) {
    if (idx > v->head.part_count - 1) {
        // We can't move a part that doesn't exist.
        LOG_MSG(error, "Cant move part %d, there are only %d parts!\n", idx, v->head.part_count);
        return false;
    }
    part_entry* p = &v->parts[idx];
    bool needed_readjustment = false;

    // We loop over the 3 axes here
    for (u8 i = 0; i < 3; i++) {
        // Find position of this axis after the move
        s16 new_pos = (s16)p->pos.raw[i] + diff.raw[i];
        if (new_pos >= 0) {
            // Everything's fine, update pos and move on
            p->pos.raw[i] += diff.raw[i];
            continue;
        }

        needed_readjustment = true;
        // Make this the new 0, and adjust the rest of the parts
        p->pos.raw[i] = 0;
        for (u16 j = 0; j < v->head.part_count; j++) {
            if (j == idx) {
                // This is the part we just made the new 0, skip.
                continue;
            }

            if (v->parts[i].pos.raw[i] >= UINT8_MAX - new_pos) {
                // Integer overflow, we can't move this part any further.
                continue;
            }

            // Pull each part back on this axis by however much we're out of bounds
            v->parts[j].pos.raw[i] -= new_pos;
        }
    }

    // Return bool result on if the part moved out of bounds and had to be adjusted
    return needed_readjustment;
}

part_entry* part_by_pos(vehicle* v, vec3u8 pos) {
    // Linearly search for the part
    for (u16 i = 0; i < v->head.part_count; i++) {
        part_entry* p = &v->parts[i]; // Just a shortcut
        if (vec3u8_eq(p->pos, pos)) {
            return p;
        }
    }
    return NULL;
}
