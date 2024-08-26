#include <memory.h>
#include <math.h>

#include <common/vector.h>
#include <common/int.h>

#include <parts.h>
#include "vehicle_edit.h"
#include "editor.h"
#include "camera.h"

bool vehiclemask_get_3d(vehicle_bitmask* mask, vec3s8 cell) {
    u8* mask_addr = (u8*)&(*mask)[cell.x][cell.y]; // Target byte

    // Only try to access mask bits if the address is in bounds.
    if (mask_addr >= (u8*)mask && mask_addr <= (u8*)mask[1]) {
        // Get mask value
        return mask_get(mask_addr, cell.z);
    }

    // Out of bounds? Zero.
    return 0;
}

void vehiclemask_set_3d(vehicle_bitmask* mask, vec3s8 cell, u8 val) {
    u8* mask_addr = (u8*)&(*mask)[cell.x][cell.y]; // Target byte
    val &= 1; // Only keep the lowest bit of the value.

    // Only try to set mask bits if the address is in bounds.
    if (mask_addr >= (u8*)mask && mask_addr <= (u8*)mask[1]) {
        // Set mask value
        mask_set(mask_addr, cell.z, val);
    }

    // Oh well.
}

bool cell_is_selected(editor_state* editor, vec3s8 cell) {
    if (!vehiclemask_get_3d(editor->selected_mask, cell)) {
        return false;
    }
    // Even if the cell is marked as selected, it might just be overlapping a
    // selected part. We need to double-check.
    // TODO: part_by_pos() can't distinguish between 2 parts in the same spot...
    u16 idx = part_by_pos(editor->v, cell);
    return list_contains(editor->selected_parts, (void*)&idx);
}

bool vehicle_part_conflict(vehicle_bitmask* vacancy, part_entry* p) {
    bool result = false;
    part_cell_iterator iter = part_cell_iterator_setup(*p);
    while (!iter.done) {
        // Get the final coordinate by adding the rotated point to origin
        vec3s8 cell = part_cell_iterator_next(&iter);

        bool cell_occupied = vehiclemask_get_3d(vacancy, cell);
        result |= cell_occupied;

    }
    return result;
}

bool vehicle_selection_overlap(editor_state* editor) {
    vehicle* v = editor->v;
    for (u32 i = 0; i < editor->selected_parts.end_idx; i++) {
        u16 idx = ((u16*)editor->selected_parts.data)[i];
        if (vehicle_part_conflict(editor->vacancy_mask, &v->parts[idx])) {
            return true;
        }
    }
    return false;
}

void update_vacancymask(editor_state* editor) {
    vehicle* v = editor->v; // Just a shorthand
    // Clear the selection grid
    memset(editor->vacancy_mask, 0x00, sizeof(vehicle_bitmask));

    for (u16 i = 0; i < v->head.part_count; i++) {
        part_entry p = v->parts[i];

        // This is kind of inefficient, but it should be ok...
        // TODO: This is a place where a list of unselected parts would be nice
        if (list_contains(editor->selected_parts, (void*)&i)) {
            // Skip selected parts, we only want to update unselected ones
            continue;
        }

        part_cell_iterator iter = part_cell_iterator_setup(p);
        while (!iter.done) {
            // Get the next cell of this part
            vec3s8 cell = part_cell_iterator_next(&iter);

            vehiclemask_set_3d(editor->vacancy_mask, cell, true);
        }
    }
}

void update_selectionmask(editor_state* editor) {
    vehicle* v = editor->v; // Just a shorthand
    // Clear the selection grid
    memset(editor->selected_mask, 0x00, sizeof(vehicle_bitmask));

    for (u16 i = 0; i < editor->selected_parts.end_idx; i++) {
        u16 idx = ((u16*)editor->selected_parts.data)[i]; // Selected list stores indices
        part_entry p = v->parts[idx];

        // This is kind of inefficient, but it should be ok...
        bool selected = true;

        part_cell_iterator iter = part_cell_iterator_setup(p);
        while (!iter.done) {
            // Get the next cell of this part
            vec3s8 cell = part_cell_iterator_next(&iter);

            vehiclemask_set_3d(editor->selected_mask, cell, true);
        }
    }
}

vec3s16 vehicle_selection_center(editor_state* editor) {
    vehicle* v = editor->v;
    vec3s8 sel_max = {0}; // Highest position in the selection
    vec3s8 sel_min = {127, 127, 127}; // Smallest position in the selection
    for (u32 i = 0; i < editor->selected_parts.end_idx; i++) {
        u16 idx = ((u16*)editor->selected_parts.data)[i];
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

    bool needed_adjust = false;
    for (u32 i = 0; i < editor->selected_parts.end_idx; i++) {
        // Selected parts list stores indices, sorry it's a little confusing
        const u32 idx = ((u16*)editor->selected_parts.data)[i];
        part_entry* p = &v->parts[idx];

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
        needed_adjust |= vehicle_move_part(editor->v, idx, diff, &adjustment);
        // If we tried to cross the edge and parts were adjusted, we need to
        // adjust the centerpoint. (will be zero if no adjustment was needed)
        editor->sel_box.x -= adjustment.x;
        editor->sel_box.y -= adjustment.y;
        editor->sel_box.z -= adjustment.z;
    }

    return needed_adjust;
}

// Setup an iterator from a part entry. Returns an iteration context.
part_cell_iterator part_cell_iterator_setup(part_entry p) {
    part_cell_iterator out = {
        .info = part_get_info(p.id),
        .part = p,
        .done = false,
    };
    return out;
}

// Get the next item and advance.
vec3s8 part_cell_iterator_next(part_cell_iterator* ctx) {
    // Early exit if iteration is already finished
    if (ctx->done) {
        return (vec3s8){0, 0, 0};
    }

    // Get the origin and current relative cell we're working with
    vec3s relative_cell = vec3_from_vec3s8(*ctx->info.relative_occupation, 1.0f);
    vec3s8 origin = ctx->part.pos;

    // Get quaternion of part rotation
    // Sorry for the ugly cast, this is just making it treat a vec3 as vec3s because they're the same
    versors quaternion = glms_euler_xyz_quat(*(vec3s*)&ctx->part.rot);

    // Rotate the relative point about the part origin
    vec3s rotated_point = glms_quat_rotatev(quaternion, relative_cell);

    // Get the final coordinate by adding the relative rotated point to origin
    vec3s8 cell = {
        MAX(origin.x + roundf(rotated_point.x), 0),
        MAX(origin.y + roundf(rotated_point.y), 0),
        MAX(origin.z + roundf(rotated_point.z), 0),
    };

    // Array ends with an all-zero entry, so if we just hit that, we're done
    ctx->done = vec3s8_eq(*ctx->info.relative_occupation, (vec3s8){0});

    // Increment the current position
    ctx->info.relative_occupation++;

    return cell;
}

s32 part_by_pos(vehicle* v, vec3s8 target) {
    // Linearly search for the part
    // TODO: When moving a selection through other parts, the detected part can
    // suddenly switch to one of the parts you're moving through. Maybe we
    // should prioritize selected parts in the search?
    // TODO: Use the grid for an early exit
    for (u16 i = 0; i < v->head.part_count; i++) {
        part_entry* p = &v->parts[i]; // Just a shortcut

        // A part's max width is 8, so anything further away can't be a match
        if (abs(p->pos.x - target.x) > 8 || 
            abs(p->pos.y - target.y) > 8 || 
            abs(p->pos.z - target.z) > 8) {
            continue; // The part is too far away, skip it
        }

        // Loop over every cell this part occupies
        part_cell_iterator iter = part_cell_iterator_setup(*p);
        if (p->id == WHEEL_SUPER && vec3s8_eq(target, p->pos)) {
            printf("");
        }
        while (!iter.done) {
            // Get the next coordinate
            vec3s8 cell = part_cell_iterator_next(&iter);

            if (vec3s8_eq(cell, target)) {
                // Found it!
                return i;
            }
        }
    }

    // Nothing here...
    return -1;
}

