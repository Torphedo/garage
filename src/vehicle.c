#include <stdbool.h>
#include <malloc.h>

#include "common/endian.h"
#include "common/file.h"

#include "vehicle.h"
#include "common/vector.h"
#include "parts.h"
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

void vehicle_unselect_all(vehicle* v) {
    for (u16 i = 0; i < v->head.part_count; i++) {
        v->parts[i].selected = false;
    }
}

bool vehicle_part_conflict(vehicle_bitmask* vacancy, part_entry* p) {
    vec3s8 empty = {0};
    part_id id = p->id;
    // Find all the cells this part contains
    vec3s8* positions = part_get_info(id).relative_occupation;
    if (positions == NULL) {
        positions = &empty;
    }

    vec3s8 origin = p->pos;
    bool result = false;
    while (1) {
        u8* vacancy_addr = (u8*)&(*vacancy)[origin.x + positions->x][origin.y + positions->y];
        bool cell_occupied = mask_get(vacancy_addr, origin.z + positions->z);

        // If the part
        result |= cell_occupied;
        if (vec3s8_eq(*positions, empty)) {
            break;
        }
        positions++;
    }
    return result;
}

bool vehicle_selection_overlap(vehicle* v, vehicle_bitmask* vacancy_mask) {
    for (u16 i = 0; i < v->head.part_count; i++) {
        if (!v->parts[i].selected) {
            continue;
        }
        if (vehicle_part_conflict(vacancy_mask, &v->parts[i])) {
            return true;
        }
    }
    return false;
}

void update_vehiclemask(vehicle* v, vehicle_bitmask* vacancy, vehicle_bitmask* selection) {
    vec3s8 empty = {0};
    // Erase the bitmask
    memset(vacancy, 0x00, sizeof(vehicle_bitmask));
    memset(selection, 0x00, sizeof(vehicle_bitmask));

    for (u16 i = 0; i < v->head.part_count; i++) {
        // Get the list of points relative to the origin this part occupies.
        part_id id = v->parts[i].id;
        vec3s8* positions = part_get_info(id).relative_occupation;
        if (positions == NULL) {
            positions = &empty;
        }

        vec3s8 origin = v->parts[i].pos;
        while (1) {
            vec3s8 cell = {
                MAX(origin.x + positions->x, 0),
                MAX(origin.y + positions->y, 0),
                MAX(origin.z + positions->z, 0),
            };
            u8* vacancy_addr = (u8*)&(*vacancy)[cell.x][cell.y];
            u8* selection_addr = (u8*)&(*selection)[cell.x][cell.y];
            bool selected = v->parts[i].selected;

            // Only try to set mask bits if the address is in bounds.
            if (vacancy_addr >= (u8*)vacancy && vacancy_addr <= (u8*)vacancy + sizeof(*vacancy)) {
                // Remove selected parts from vacancy mask & add them to the
                // selection mask
                mask_set(vacancy_addr, cell.z, !selected);
                mask_set(selection_addr, cell.z, selected);
            }

            // The position array ends with an all-zero entry (the origin)
            if (vec3s8_eq(*positions, empty)) {
                break;
            }
            positions++;
        }
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

part_entry* part_by_pos(vehicle* v, vec3s8 pos) {
    // Linearly search for the part
    for (u16 i = 0; i < v->head.part_count; i++) {
        part_entry* p = &v->parts[i]; // Just a shortcut
        if (vec3s8_eq(p->pos, pos)) {
            return p;
        }
    }
    return NULL;
}
