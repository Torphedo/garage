#include <memory.h>

#include <common/vector.h>

#include "../parts.h"
#include "vehicle_edit.h"
#include "camera.h"

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

vec3s8 vehicle_selection_center(vehicle* v) {
    vec3s8 sel_max = {0}; // Highest position in the selection
    vec3s8 sel_min = {127, 127, 127}; // Smallest position in the selection
    for (u16 i = 0; i < v->head.part_count; i++) {
        part_entry* p = &v->parts[i];
        if (!p->selected) {
            continue;
        }

        // Update selection min/max positions
        sel_max = (vec3s8) {
            MAX(sel_max.x, p->pos.x),
            MAX(sel_max.y, p->pos.y),
            MAX(sel_max.z, p->pos.z),
        };
        sel_min = (vec3s8) {
            MIN(sel_min.x, p->pos.x),
            MIN(sel_min.y, p->pos.y),
            MIN(sel_min.z, p->pos.z),
        };
    }
    vec3s8 sel_center = {
        floorf((float)(sel_max.x + sel_min.x) / 2),
        floorf((float)(sel_max.y + sel_min.y) / 2),
        floorf((float)(sel_max.z + sel_min.z) / 2),
    };

    return sel_center;
}

bool vehicle_rotate_selection(gui_state* gui, s8 forward_diff, s8 side_diff, s8 roll_diff) {
    vehicle* v = gui->v;

    vec3s cam_view = glms_normalize(camera_facing());
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
    glm_rotate(rot_matrix, glm_rad(90) * roll_diff, *(vec3*)&forward_vec);

    // In this loop we just find selected part count
    u16 part_count = 0;
    for (u16 i = 0; i < v->head.part_count; i++) {
        part_entry* p = &v->parts[i];
        if (!vehiclemask_get_3d(gui->selected_mask, p->pos.x, p->pos.y, p->pos.z)) {
            continue; // Skip unselected parts
        }
        part_count++;
    }
    vec3s8 sel_center = (vec3s8){
        MAX(gui->sel_box.x, 0),
        MAX(gui->sel_box.y, 0),
        MAX(gui->sel_box.z, 0),
    };
    vehiclemask_set_3d(gui->vacancy_mask, sel_center.x, sel_center.y, sel_center.z, 1);

    for (u16 i = 0; i < v->head.part_count; i++) {
        part_entry* p = &v->parts[i];
        if (!vehiclemask_get_3d(gui->selected_mask, p->pos.x, p->pos.y, p->pos.z)) {
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
            (float)p->pos.x - sel_center.x,
            (float)p->pos.y - sel_center.y,
            (float)p->pos.z - sel_center.z,
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
