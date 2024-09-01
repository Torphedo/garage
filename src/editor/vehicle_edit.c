#include <memory.h>
#include <math.h>

#include <common/vector.h>
#include <common/int.h>
#include <common/logging.h>

#include <parts.h>
#include "vehicle_edit.h"
#include "common/list.h"
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
    // Try to find a part from the selected list at this position
    const part_entry* p = part_by_pos(editor, cell, SEARCH_SELECTED);

    // An id of 0 indicates placeholder, meaning nothing was found
    const bool found_part = (p->id != 0);

    return found_part;
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
    part_iterator iter = part_iterator_setup(*editor, SEARCH_SELECTED);
    while (!iter.done) {
        part_entry* p = part_iterator_next(&iter);
        if (vehicle_part_conflict(editor->vacancy_mask, p)) {
            return true;
        }
    }
    return false;
}

void update_vacancymask(editor_state* editor) {
    // Clear the selection grid
    memset(editor->vacancy_mask, 0x00, sizeof(vehicle_bitmask));

    part_iterator iter = part_iterator_setup(*editor, SEARCH_UNSELECTED);
    while (!iter.done) {
        part_entry* p = part_iterator_next(&iter);

        part_cell_iterator cell_iter = part_cell_iterator_setup(*p);
        while (!cell_iter.done) {
            // Get the next cell of this part
            vec3s8 cell = part_cell_iterator_next(&cell_iter);

            vehiclemask_set_3d(editor->vacancy_mask, cell, true);
        }
    }
}

void update_selectionmask(editor_state* editor) {
    // Clear the selection grid
    memset(editor->selected_mask, 0x00, sizeof(vehicle_bitmask));

    part_iterator iter = part_iterator_setup(*editor, SEARCH_SELECTED);
    while (!iter.done) {
        part_entry* p = part_iterator_next(&iter);

        part_cell_iterator cell_iter = part_cell_iterator_setup(*p);
        while (!cell_iter.done) {
            // Get the next cell of this part
            vec3s8 cell = part_cell_iterator_next(&cell_iter);

            vehiclemask_set_3d(editor->selected_mask, cell, true);
        }
    }
}

vec3s vehicle_find_center(const editor_state* editor, partsearch_type search_type) {
    vec3s8 max = {0}; // Highest position in the selection
    vec3s8 min = {127, 127, 127}; // Smallest position in the selection

    part_iterator iter = part_iterator_setup(*editor, search_type);
    while (!iter.done) {
        const part_entry* p = part_iterator_next(&iter);
        part_cell_iterator cell_iter = part_cell_iterator_setup(*p);
        while (!cell_iter.done) {
            const vec3s8 pos = part_cell_iterator_next(&cell_iter);
            // Update the min/max positions
            min = (vec3s8) {
                MIN(min.x, pos.x),
                MIN(min.y, pos.y),
                MIN(min.z, pos.z),
            };
            max = (vec3s8) {
                MAX(max.x, pos.x),
                MAX(max.y, pos.y),
                MAX(max.z, pos.z),
            };
        }
    }

    vec3s center = {
        (float)(max.x + min.x) / 2,
        (float)(max.y + min.y) / 2,
        (float)(max.z + min.z) / 2,
    };

    return center;
}


bool vehicle_rotate_selection(editor_state* editor, s8 forward_diff, s8 side_diff, s8 roll_diff) {
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
    part_iterator iter = part_iterator_setup(*editor, SEARCH_SELECTED);
    while (!iter.done) {
        part_entry* p = part_iterator_next(&iter);

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
        needed_adjust |= vehicle_move_part(editor, *p, diff, &adjustment);
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
    // Get the origin and current relative cell we're working with
    const vec3s relative_cell = vec3_from_vec3s8(ctx->info.relative_occupation[ctx->cell_idx], 1.0f);
    const vec3s8 origin = ctx->part.pos;

    // Get quaternion of part rotation
    // Sorry for the ugly cast, this is just making it treat a vec3 as vec3s because they're the same
    const versors quaternion = glms_euler_xyz_quat(*(vec3s*)&ctx->part.rot);

    // Rotate the relative point about the part origin
    const vec3s rotated_point = glms_quat_rotatev(quaternion, relative_cell);

    // Get the final coordinate by adding the relative rotated point to origin
    const vec3s8 cell = {
        MAX(origin.x + roundf(rotated_point.x), 0),
        MAX(origin.y + roundf(rotated_point.y), 0),
        MAX(origin.z + roundf(rotated_point.z), 0),
    };

    // Array ends with an all-zero entry, so if we just hit that, we're done
    // TODO: We can also do a bounds check against max volume here
    ctx->done = glms_vec3_eqv(relative_cell, (vec3s){0});

    // Increment the current position if needed
    if (!ctx->done) {
        ctx->cell_idx++;
    }

    return cell;
}

part_iterator part_iterator_setup(editor_state editor, partsearch_type search_type) {
    part_iterator output = {
        .partlists = {editor.selected_parts, editor.unselected_parts},
        .search_type = search_type,
    };

    switch (search_type) {
        case SEARCH_SELECTED:
            // No special action needed, but make both entries the selected
            // list just in case
            output.partlists[1] = editor.selected_parts;
            break;
        case SEARCH_UNSELECTED:
            // Make both entries the unselected list
            output.partlists[0] = editor.unselected_parts;
            break;
        case SEARCH_ALL:
            // No special action needed
            break;
    }

    // Skip empty lists
    list cur_list = output.partlists[output.partlist_idx];
    if (list_empty(cur_list)) {
        output.partlist_idx++;
        cur_list = output.partlists[output.partlist_idx];
        if (list_empty(cur_list)) {
            output.partlist_idx++;
        }
    }

    // Mark as done on creation if there's nothing left
    const u8 max_partlist_idx = output.search_type == SEARCH_ALL;
    if (output.partlist_idx > max_partlist_idx || list_empty(cur_list)) {
        output.done = true;
    }

    return output;
}

part_entry* part_iterator_next(part_iterator* ctx) {
    list cur_list = ctx->partlists[ctx->partlist_idx];
    part_entry* part = (part_entry*)list_get_element(cur_list, ctx->part_idx);
    ctx->part_idx++;

    // Move on to the next list if needed
    if (ctx->part_idx >= cur_list.end_idx) {
        ctx->part_idx = 0;
        ctx->partlist_idx++;
        cur_list = ctx->partlists[ctx->partlist_idx];
    }

    const u8 max_partlist_idx = ctx->search_type == SEARCH_ALL;
    if (ctx->partlist_idx > max_partlist_idx || list_empty(cur_list)) {
        ctx->done = true;
    }

    return part;
}

static part_entry empty_part = {0};

part_entry* part_by_pos(editor_state* editor, vec3s8 target, partsearch_type search_hint) {
    const bool vacancy_result = vehiclemask_get_3d(editor->vacancy_mask, target);
    const bool selection_result = vehiclemask_get_3d(editor->selected_mask, target);
    if (!vacancy_result && !selection_result) {
        // This cell isn't in the selection or vacancy grid, so there's no part here.
        empty_part = (part_entry){0};
        return &empty_part;
    }

    // Linearly search for the part
    part_iterator iter = part_iterator_setup(*editor, search_hint);
    while (!iter.done) {
        part_entry* part = part_iterator_next(&iter);

        // A part's max width is 8, so anything further away can't be a match
        if (abs(part->pos.x - target.x) > 8 || 
            abs(part->pos.y - target.y) > 8 || 
            abs(part->pos.z - target.z) > 8) {
            continue; // The part is too far away, skip it
        }

        // Loop over every cell this part occupies
        part_cell_iterator cell_iter = part_cell_iterator_setup(*part);
        while (!cell_iter.done) {
            // Get the next coordinate
            vec3s8 cell = part_cell_iterator_next(&cell_iter);

            if (vec3s8_eq(cell, target)) {
                // Found it!
                return part;
            }
        }
    }

    // Nothing here...
    empty_part = (part_entry){0}; // Make sure we return an empty part
    return &empty_part;
}

bool vehicle_move_part(editor_state* editor, part_entry part, vec3s16 diff, vec3s16* adjust_out) {
    s64 idx = list_find(editor->selected_parts, &part);
    if (idx == -1) {
        // If for some reason we're moving an unselected part, handle that
        idx = list_find(editor->unselected_parts, &part);
        if (idx == -1) {
            return false;
        }
    }
    part_entry* p = (part_entry*) list_get_element(editor->selected_parts, idx);

    bool needed_readjustment = false;
    // We loop over the 3 axes here
    for (u8 i = 0; i < 3; i++) {
        // Find position of this axis after the move
        const s16 new_pos = (s16)p->pos.raw[i] + diff.raw[i];
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

        part_iterator iter = part_iterator_setup(*editor, SEARCH_ALL);
        while (!iter.done) {
            
            // The part to be moved
            part_entry* other_part = part_iterator_next(&iter);

            if (memcmp(p, other_part, sizeof(part_entry)) == 0) {
                // This is the part we just made the new 0, skip.
                continue;
            }

            if (other_part->pos.raw[i] >= VEH_MAX_DIM - new_pos) {
                // Integer overflow, we can't move this part any further.
                other_part->pos.raw[i] = VEH_MAX_DIM - 1;
                continue;
            }

            // Pull each part back on this axis by however much we're out of bounds
            other_part->pos.raw[i] -= new_pos;
        }
    }

    // Return bool result on if the part moved out of bounds and had to be adjusted
    return needed_readjustment;
}


