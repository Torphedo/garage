#include <memory.h>
#include <math.h>

#include <common/vector.h>
#include <common/int.h>

#include <parts.h>
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

bool cell_is_selected(editor_state* editor, vec3s8 cell) {
    if (!vehiclemask_get_3d(editor->selected_mask, cell.x, cell.y, cell.z)) {
        return false;
    }
    // Even if the cell is marked as selected, it might just be overlapping a
    // selected part. We need to double-check.
    u16 idx = part_by_pos(editor->v, cell);
    for (u32 i = 0; i < editor->selected_parts.end_idx; i++) {
        if (idx == editor->selected_parts.data[i]) {
            // It's in the list of selected parts, so it's definitely selected
            return true;
        }
    }

    // Found nothing, it was just an overlap.
    return false;
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

bool vehicle_selection_overlap(editor_state* editor) {
    vehicle* v = editor->v;
    for (u32 i = 0; i < editor->selected_parts.end_idx; i++) {
        u16 idx = editor->selected_parts.data[i];
        if (vehicle_part_conflict(editor->vacancy_mask, &v->parts[idx])) {
            return true;
        }
    }
    return false;
}

void update_vehiclemask(vehicle* v, list selected_parts, vehicle_bitmask* vacancy, vehicle_bitmask* selection) {
    // Erase the bitmask
    memset(vacancy, 0x00, sizeof(vehicle_bitmask));
    memset(selection, 0x00, sizeof(vehicle_bitmask));

    for (u16 i = 0; i < v->head.part_count; i++) {
        // Get the list of points relative to the origin this part occupies.
        part_entry p = v->parts[i];
        part_id id = p.id;
        // This is kind of inefficient, but it should be ok...
        bool selected = list_contains(&selected_parts, i);

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

vec3s16 vehicle_selection_center(editor_state* editor) {
    vehicle* v = editor->v;
    vec3s8 sel_max = {0}; // Highest position in the selection
    vec3s8 sel_min = {127, 127, 127}; // Smallest position in the selection
    for (u32 i = 0; i < editor->selected_parts.end_idx; i++) {
        u16 idx = editor->selected_parts.data[i];
        part_entry* p = &v->parts[idx];

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
    vec3s16 sel_center = {
        floorf((float)(sel_max.x + sel_min.x) / 2),
        floorf((float)(sel_max.y + sel_min.y) / 2),
        floorf((float)(sel_max.z + sel_min.z) / 2),
    };

    return sel_center;
}

bool vehicle_rotate_selection(editor_state* editor, s8 forward_diff, s8 side_diff, s8 roll_diff) {
    vehicle* v = editor->v;

    vec3s cam_view = glms_normalize(camera_facing(editor->cam));
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

    for (u32 i = 0; i < editor->selected_parts.end_idx; i++) {
        // Selected parts list stores indices, sorry it's a little confusing
        part_entry* p = &v->parts[editor->selected_parts.data[i]];

        // Get rotation matrix for the part rotation
        mat4 part_rotation = {0};
        glm_euler(p->rot, part_rotation);

        // Combine rotation matrices & update the part rotation
        glm_mat4_mul(rot_matrix, part_rotation, part_rotation);
        glm_euler_angles(part_rotation, p->rot);

        vec3 offset = {
            (float)p->pos.x - editor->sel_box.x,
            (float)p->pos.y - editor->sel_box.y,
            (float)p->pos.z - editor->sel_box.z,
        };
        vec3 rotated_offset = {0};
        vec4 quaternion = {0};
        glm_mat4_quat(rot_matrix, quaternion);
        glm_quat_rotatev(quaternion, offset, rotated_offset);

        // Part position after rotation
        vec3s16 new_pos = {
            roundf(editor->sel_box.x + rotated_offset[0]),
            roundf(editor->sel_box.y + rotated_offset[1]),
            roundf(editor->sel_box.z + rotated_offset[2]),
        };
        vec3s16 diff = {
            new_pos.x - p->pos.x,
            new_pos.y - p->pos.y,
            new_pos.z - p->pos.z,
        };
        vec3s16 adjustment = {0};
        vehicle_move_part(editor->v, editor->selected_parts.data[i], diff, &adjustment);
        // If we tried to cross the edge and parts were adjusted, we need to
        // adjust the centerpoint. (will be zero if no adjustment was needed)
        editor->sel_box.x -= adjustment.x;
        editor->sel_box.y -= adjustment.y;
        editor->sel_box.z -= adjustment.z;
    }

    return true;
}

