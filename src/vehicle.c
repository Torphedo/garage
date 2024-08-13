#include <stdbool.h>
#include <malloc.h>

#include <common/endian.h>
#include <common/file.h>
#include <common/vector.h>
#include <common/logging.h>
#include <cglm/cglm.h>

#include "vehicle.h"
#include "parts.h"
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
    ENDIAN_FLIP_FLOAT(v->weight);
    ENDIAN_FLIP_FLOAT(v->unknownf);
    ENDIAN_FLIP_FLOAT(v->unknownf_1);
}

vec3s vehicle_find_center(vehicle* v) {
    vec3s8 max = {0}; // Max XYZ position found in the vehicle
    for (u16 i = 0; i < v->head.part_count; i++) {
        vec3s8 pos = v->parts[i].pos;

        // Update the max position if the part's position is larger on any axis
        max = (vec3s8) {
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
        if (new_pos >= VEH_MAX_DIM - 1) {
            // This part is at the border, there's nothing we can do.
            continue;
        }
        else if (new_pos >= 0) {
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

            if (v->parts[i].pos.raw[i] >= VEH_MAX_DIM - new_pos) {
                // Integer overflow, we can't move this part any further.
                v->parts[i].pos.raw[i] = VEH_MAX_DIM - 1;
                continue;
            }

            // Pull each part back on this axis by however much we're out of bounds
            v->parts[j].pos.raw[i] -= new_pos;
        }
    }

    // Return bool result on if the part moved out of bounds and had to be adjusted
    return needed_readjustment;
}

part_entry empty_part = {0};

s32 part_by_pos(vehicle* v, vec3s8 target) {
    // Linearly search for the part
    // TODO: When moving a selection through other parts, the detected part can
    // suddenly switch to one of the parts you're moving through. Maybe we
    // should prioritize selected parts in the search?
    for (u16 i = 0; i < v->head.part_count; i++) {
        part_entry* p = &v->parts[i]; // Just a shortcut

        // A part's max width is 8, so anything further away can't be a match
        if (abs(p->pos.x - target.x) > 8 || 
            abs(p->pos.y - target.y) > 8 || 
            abs(p->pos.z - target.z) > 8) {
            continue; // The part is too far away, skip it
        }

        part_info part = part_get_info(p->id);
        // Get quaternion of part rotation
        // Sorry for the ugly cast, this is just making it treat a vec3 as vec3s because they're the same
        versors quaternion = glms_euler_xyz_quat(*(vec3s*)&p->rot);

        // Loop over every cell this part occupies
        while (1) {
            // Rotate the relative point around the part's origin
            vec3s rotated_point = glms_quat_rotatev(quaternion, vec3_from_vec3s8(*part.relative_occupation, 1.0f));

            // Get the final coordinate by adding the rotated point to origin
            vec3s8 cell = {
                MAX(p->pos.x + roundf(rotated_point.x), 0),
                MAX(p->pos.y + roundf(rotated_point.y), 0),
                MAX(p->pos.z + roundf(rotated_point.z), 0),
            };

            if (vec3s8_eq(cell, target)) {
                // Found it!
                return i;
            }

            // Move on when we find the final entry (all zeroes, the origin)
            if (vec3s8_eq(*part.relative_occupation, (vec3s8){0})) {
                break; // Breaks from the while() loop, not the for()
            }
            part.relative_occupation++; // Go to next entry
        }
    }

    // Nothing here...
    return -1;
}
