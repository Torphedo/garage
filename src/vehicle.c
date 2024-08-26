#include <stdlib.h>
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

void part_byteswap(part_entry* part) {
    if (is_little_endian()) {
        ENDIAN_FLIP(u32, part->unknown);
        ENDIAN_FLIP(u32, part->id);
        vec3_byteswap(&part->rot);

        // Byte-swap the padding just in case :P
        ENDIAN_FLIP(u16, part->pad);
        ENDIAN_FLIP(u32, part->pad3);
    }
}

void vehicle_header_byteswap(vehicle_header* v) {
    if (is_little_endian()) {
        ENDIAN_FLIP(u64, v->magic);
        ENDIAN_FLIP(u16, v->part_count);
        for (u32 i = 0; i < ARRAY_SIZE(v->name); i++) {
            // 16-bit characters are also subject to endian-ness
            ENDIAN_FLIP(c16, v->name[i]);
        }
        ENDIAN_FLIP_FLOAT(v->weight);
        ENDIAN_FLIP_FLOAT(v->unknownf);
        ENDIAN_FLIP_FLOAT(v->unknownf_1);
    }
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

bool vehicle_move_part(vehicle* v, u16 idx, vec3s16 diff, vec3s16* adjust_out) {
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
        // Make this the new 0, update the output adjustment vector, and adjust
        // the rest of the parts
        p->pos.raw[i] = 0;
        if (adjust_out != NULL) {
            adjust_out->raw[i] = new_pos;
        }
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

