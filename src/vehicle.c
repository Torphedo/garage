#include <stdbool.h>
#include <malloc.h>

#include <cglm/struct.h>
#include "cglm/mat4.h"
#include "cglm/quat.h"
#include "cglm/struct/mat4.h"
#include "common/endian.h"
#include "common/file.h"
#include "common/vector.h"
#include "common/logging.h"

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

bool vehiclemask_get_3d(vehicle_bitmask* mask, s8 x, s8 y, s8 z) {
    u8* mask_addr = (u8*)&(*mask)[x][y];

    // Only try to access mask bits if the address is in bounds.
    if (mask_addr >= (u8*)mask && mask_addr <= (u8*)mask[1]) {
        // Get mask value
        return mask_get(mask_addr, z);
    }

    // Out of bounds? Zero.
    return 0;
}

void vehiclemask_set_3d(vehicle_bitmask* mask, s8 x, s8 y, s8 z, u8 val) {
    u8* mask_addr = (u8*)&(*mask)[x][y];
    val &= 1; // Only keep the lowest bit of the value.

    // Only try to set mask bits if the address is in bounds.
    if (mask_addr >= (u8*)mask && mask_addr <= (u8*)mask[1]) {
        // Set mask value
        mask_set(mask_addr, z, val);
    }

    // Oh well.
}

void vehicle_unselect_all(vehicle* v) {
    for (u16 i = 0; i < v->head.part_count; i++) {
        v->parts[i].selected = false;
    }
}

bool vehicle_part_conflict(vehicle_bitmask* vacancy, part_entry* p) {
    part_id id = p->id;
    // Find all the cells this part contains
    vec3s8* positions = part_get_info(id).relative_occupation;

    // Get quaternion of part rotation
    // Sorry for the ugly cast, this is just making it treat a vec3 as vec3s because they're the same
    versors quaternion = glms_euler_xyz_quat(*(vec3s*)&p->rot);

    vec3s8 origin = p->pos;
    bool result = false;
    while (1) {
        // Rotate the relative point around the part's origin
        vec3s rotated_point = glms_quat_rotatev(quaternion, vec3_from_vec3s8(*positions, 1.0f));

        // Get the final coordinate by adding the rotated point to origin
        vec3s8 cell = {
            MAX(origin.x + roundf(rotated_point.x), 0),
            MAX(origin.y + roundf(rotated_point.y), 0),
            MAX(origin.z + roundf(rotated_point.z), 0),
        };

        bool cell_occupied = vehiclemask_get_3d(vacancy, cell.x, cell.y, cell.z);
        result |= cell_occupied;

        // The position array ends with an all-zero entry (the origin)
        if (vec3s8_eq(*positions, (vec3s8){0})) {
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
    // Erase the bitmask
    memset(vacancy, 0x00, sizeof(vehicle_bitmask));
    memset(selection, 0x00, sizeof(vehicle_bitmask));

    for (u16 i = 0; i < v->head.part_count; i++) {
        // Get the list of points relative to the origin this part occupies.
        part_entry p = v->parts[i];
        part_id id = p.id;
        vec3s8* positions = part_get_info(id).relative_occupation;
        // Get quaternion of part rotation
        // Sorry for the ugly cast, this is just making it treat a vec3 as vec3s because they're the same
        versors quaternion = glms_euler_xyz_quat(*(vec3s*)&p.rot);

        vec3s8 origin = p.pos;
        while (1) {
            // Rotate the relative point around the part's origin
            vec3s rotated_point = glms_quat_rotatev(quaternion, vec3_from_vec3s8(*positions, 1.0f));

            // Get the final coordinate by adding the rotated point to origin
            vec3s8 cell = {
                MAX(origin.x + roundf(rotated_point.x), 0),
                MAX(origin.y + roundf(rotated_point.y), 0),
                MAX(origin.z + roundf(rotated_point.z), 0),
            };

            bool selected = p.selected;

            // Remove selected parts from vacancy mask & add them to the
            // selection mask
            vehiclemask_set_3d(vacancy, cell.x, cell.y, cell.z, !selected);
            vehiclemask_set_3d(selection, cell.x, cell.y, cell.z, selected);

            // The position array ends with an all-zero entry (the origin)
            if (vec3s8_eq(*positions, (vec3s8){0})) {
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



bool vehicle_rotate_selection(vehicle* v, vehicle_bitmask* selection, vehicle_bitmask* vacancy, s8 forward_diff, s8 side_diff, vec3s cam_view) {
    // Absolute value of camera vector
    vec3s cam_abs = {fabsf(cam_view.x), fabsf(cam_view.y), fabsf(cam_view.z)};

    // Set our view direction to have a magnitude of 1 on the horizontal axis
    // we're facing the most strongly, and 0 in all other directions.
    vec3s horizontal_vec = {
        // zero if it's not the largest element, otherwise 1 or -1 depending on direction
        ((cam_view.x < 0) ? -1 : 1) * (cam_abs.x > cam_abs.z),
        0, // Vertical component ignored
        ((cam_view.z < 0) ? -1 : 1) * (cam_abs.z > cam_abs.x),
    };

    // Same as horizontal vector but includes vertical component
    vec3s forward_vec = {
        // zero if it's not the largest element, otherwise 1 or -1 depending on direction
        .x = ((cam_view.x < 0) ? -1 : 1) * (cam_abs.x > cam_abs.y && cam_abs.x > cam_abs.z),
        .y = ((cam_view.y < 0) ? -1 : 1) * (cam_abs.y > cam_abs.z && cam_abs.y > cam_abs.x),
        .z = ((cam_view.z < 0) ? -1 : 1) * (cam_abs.z > cam_abs.y && cam_abs.z > cam_abs.x),
    };
    vec3 right_vec = {horizontal_vec.z, 0, -horizontal_vec.x};

    // Calculate new rotations to apply to each part
    mat4 rot_matrix = {0};
    glm_mat4_identity(rot_matrix);
    glm_rotate(rot_matrix, glm_rad(90) * forward_diff, right_vec);
    if (forward_vec.x == 0 && forward_vec.z == 0) {
        // If the camera is facing mostly vertical, rotating side-to-side
        // will rotate along the most dominant horizontal camera component
        glm_rotate(rot_matrix, glm_rad(90) * side_diff, *(vec3*)&horizontal_vec);
    }
    else {
        // If the camera is mostly facing horizontally, rotating
        // side-to-side will rotate along the Y-axis
        glm_rotate(rot_matrix, glm_rad(90) * side_diff, (vec3){0, 1, 0});
    }

    // In this loop we just find part count & the selection bounding box
    vec3s8 sel_max = {0}; // Highest position in the selection
    vec3s8 sel_min = {127, 127, 127}; // Smallest position in the selection
    u16 part_count = 0;
    for (u16 i = 0; i < v->head.part_count; i++) {
        part_entry* p = &v->parts[i];
        if (!vehiclemask_get_3d(selection, p->pos.x, p->pos.y, p->pos.z)) {
            continue; // Skip unselected parts
        }
        part_count++;

        // Check all the cells this part occupies to see if they form a corner of the selection
        vec3s8* positions = part_get_info(p->id).relative_occupation;
        // Get quaternion of part rotation
        vec4 part_rot = {0};
        glm_euler_xyz_quat(p->rot, part_rot);
        vec4 user_rot = {0};
        glm_mat4_quat(rot_matrix, user_rot);

        while (1) {
            // Calculate offsets from new part rotation
            vec4 new_rot = {0};
            vec3 post_rot_offset = {0};
            glm_quat_mul(user_rot, part_rot, new_rot);
            glm_quat_rotatev(new_rot, (vec3){positions->x, positions->y, positions->z}, post_rot_offset);

            // Calculate offsets from current part rotation
            vec3 pre_rot_offset = {0};
            glm_quat_rotatev(part_rot, (vec3){positions->x, positions->y, positions->z}, pre_rot_offset);

            // Get the final coordinate by adding the rotated point to origin
            // These coordinates use the current part rotation
            vec3s8 cur_cell = {
                MAX(p->pos.x + roundf(pre_rot_offset[0]), 0),
                MAX(p->pos.y + roundf(pre_rot_offset[1]), 0),
                MAX(p->pos.z + roundf(pre_rot_offset[2]), 0),
            };

            // These coordinates use the new part rotation
            vec3s8 new_cell = {
                MAX(p->pos.x + roundf(post_rot_offset[0]), 0),
                MAX(p->pos.y + roundf(post_rot_offset[1]), 0),
                MAX(p->pos.z + roundf(post_rot_offset[2]), 0),
            };

            // Update selection min/max positions
            sel_max = (vec3s8) {
                MAX(sel_max.x, cur_cell.x),
                MAX(sel_max.y, cur_cell.y),
                MAX(sel_max.z, cur_cell.z),
            };
            sel_min = (vec3s8) {
                MIN(sel_min.x, cur_cell.x),
                MIN(sel_min.y, cur_cell.y),
                MIN(sel_min.z, cur_cell.z),
            };

            // If applying the rotation will cause a collision, bail early.
            if (vehiclemask_get_3d(vacancy, new_cell.x, new_cell.y, new_cell.z)) {
                // LOG_MSG(debug, "rotation cancelled\n");
                return false;
            }

            // The position array ends with an all-zero entry (the origin)
            if (vec3s8_eq(*positions, (vec3s8){0})) {
                break;
            }
            positions++;
        }
    }
    vec3s sel_center = {
        (float)(sel_max.x + sel_min.x) / 2,
        (float)(sel_max.y + sel_min.y) / 2,
        (float)(sel_max.z + sel_min.z) / 2,
    };

    for (u16 i = 0; i < v->head.part_count; i++) {
        part_entry* p = &v->parts[i];
        if (!vehiclemask_get_3d(selection, p->pos.x, p->pos.y, p->pos.z)) {
            continue; // Skip unselected parts
        }

        // Get rotation matrix for the part rotation
        mat4 part_rotation = {0};
        glm_euler(p->rot, part_rotation);

        // Combine rotation matrices & update the part rotation
        glm_mat4_mul(rot_matrix, part_rotation, part_rotation);
        glm_euler_angles(part_rotation, p->rot);

        if (part_count < 2) {
            // If there's only one part, we've already rotated it and shouldn't move it
            return true;
        }

        vec3 offset = {
            p->pos.x - sel_center.x,
            p->pos.y - sel_center.y,
            p->pos.z - sel_center.z,
        };
        vec3 rotated_offset = {0};
        vec4 quaternion = {0};
        glm_mat4_quat(rot_matrix, quaternion);
        glm_quat_rotatev(quaternion, offset, rotated_offset);

        p->pos = (vec3s8) {
             roundf(sel_center.x + rotated_offset[0]),
             roundf(sel_center.y + rotated_offset[1]),
             roundf(sel_center.z + rotated_offset[2]),
        };
    }

    return true;
}

part_entry empty_part = {0};

part_entry* part_by_pos(vehicle* v, vec3s8 target) {
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
                return p;
            }

            // Move on when we find the final entry (all zeroes, the origin)
            if (vec3s8_eq(*part.relative_occupation, (vec3s8){})) {
                break; // Breaks from the while() loop, not the for()
            }
            part.relative_occupation++; // Go to next entry
        }
    }

    // Nothing here...
    return &empty_part;
}
