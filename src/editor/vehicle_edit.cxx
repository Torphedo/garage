#include <memory.h>
#include <cmath>

#include <common/int.h>
#include <common/list.h>
#include <algorithm>

extern "C" {
#include <vector.h>
#include <parts.h>
}

#include "vehicle_edit.hxx"
#include "layer_editor.hxx"
#include "utils.hxx"

bool cell_matches_part(const editor_state& editor, vec3s8 target, partsearch_type type) {
    const_part_iterator iter(editor, type);
    while (!iter.done) {
        const part_entry* part = iter.next();
        part_cell_iterator cell_iter(*part);
        while (!cell_iter.done) {
            const vec3s8 cell = cell_iter.next();
            if (vec3s8_eq(cell, target)) {
                return true;
            }
        }
    }

    return false; // Nothin...
}

bool cell_is_selected(const editor_state& editor, vec3s8 target) {
    return cell_matches_part(editor, target, SEARCH_SELECTED);
}

bool cell_is_occupied(const editor_state& editor, vec3s8 target) {
    return cell_matches_part(editor, target, SEARCH_UNSELECTED);
}

bool vehicle_selection_overlap(const editor_state& editor) {
    const_part_iterator iter(editor, SEARCH_SELECTED);
    while (!iter.done) {
        const part_entry p = *iter.next();
        part_cell_iterator cell_iter(p);
        while (!cell_iter.done) {
            // Get the final coordinate by adding the rotated point to origin
            const vec3s8 cell = cell_iter.next();

            if (cell_is_occupied(editor, cell)) {
                return true;
            }

        }
    }
    return false;
}

vec3s vehicle_find_center(const editor_state* editor, partsearch_type search_type) {
    vec3s8 max = {0}; // Highest position in the selection
    vec3s8 min = {127, 127, 127}; // Smallest position in the selection

    switch (search_type) {
    case SEARCH_ALL:
    case SEARCH_SELECTED:
        for (const part_entry& p : editor->selected_parts) {
            part_cell_iterator cell_iter(p);
            while (!cell_iter.done) {
                const vec3s8 pos = cell_iter.next();
                // Update the min/max positions
                min.x = MIN(min.x, pos.x);
                min.y = MIN(min.y, pos.y);
                min.z = MIN(min.z, pos.z);

                max.x = MAX(max.x, pos.x);
                max.y = MAX(max.y, pos.y);
                max.z = MAX(max.z, pos.z);
            }
        }
        if (search_type == SEARCH_SELECTED) {
            break;
        }
        fallthrough;

    case SEARCH_UNSELECTED:
        for (const part_entry& p : editor->unselected_parts) {
            part_cell_iterator cell_iter(p);
            while (!cell_iter.done) {
                const vec3s8 pos = cell_iter.next();
                // Update the min/max positions
                min.x = MIN(min.x, pos.x);
                min.y = MIN(min.y, pos.y);
                min.z = MIN(min.z, pos.z);

                max.x = MAX(max.x, pos.x);
                max.y = MAX(max.y, pos.y);
                max.z = MAX(max.z, pos.z);
            }
        }
        break;
    }

    const vec3s center = {
        (float)(max.x + min.x) / 2,
        (float)(max.y + min.y) / 2,
        (float)(max.z + min.z) / 2,
    };

    return center;
}


bool vehicle_rotate_selection(editor_state* editor, s8 forward_diff, s8 side_diff, s8 roll_diff) {
    const vec3s cam_view = editor->cam.facing();
    // Absolute value of camera vector
    const vec3s cam_abs = {fabsf(cam_view.x), fabsf(cam_view.y), fabsf(cam_view.z)};

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
    part_iterator iter(*editor, SEARCH_SELECTED);
    while (!iter.done()) {
        part_entry* p = iter.next();

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
        vec3s8 new_pos = {
            roundf(editor->sel_box.x + rotated_offset[0]),
            roundf(editor->sel_box.y + rotated_offset[1]),
            roundf(editor->sel_box.z + rotated_offset[2]),
        };
        vec3s8 diff = {
            new_pos.x - p->pos.x,
            new_pos.y - p->pos.y,
            new_pos.z - p->pos.z,
        };
        vec3s8 adjustment = {0};
        needed_adjust |= vehicle_move_part(*editor, *p, diff, &adjustment);
        // If we tried to cross the edge and parts were adjusted, we need to
        // adjust the centerpoint. (will be zero if no adjustment was needed)
        editor->sel_box.x -= adjustment.x;
        editor->sel_box.y -= adjustment.y;
        editor->sel_box.z -= adjustment.z;
    }

    return needed_adjust;
}

vec3s8 part_cell_iterator::next() {
    // part_cell_iterator* ctx = this;
    // Get the origin and current relative cell we're working with
    const vec3s relative_cell = vec3_from_vec3s8(info.relative_occupation[cell_idx], 1.0f);
    const vec3s8 origin = part.pos;

    // Get quaternion of part rotation
    // Sorry for the ugly cast, this is just making it treat a vec3 as vec3s because they're the same
    const versors quaternion = glms_euler_xyz_quat(*(vec3s*)&part.rot);

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
    done = glms_vec3_eqv(relative_cell, vec3s{});

    // Increment the current position if needed
    if (!done) {
        cell_idx++;
    }

    return cell;
}

const_part_iterator::const_part_iterator(const editor_state& editor, partsearch_type search_type) noexcept : search_type(search_type) {
    partlists[0] = &editor.selected_parts;
    partlists[1] = &editor.unselected_parts;

    switch (search_type) {
        case SEARCH_SELECTED:
            // No special action needed, but make both entries the selected
            // list just in case
            partlists[1] = &editor.selected_parts;
            break;
        case SEARCH_UNSELECTED:
            // Make both entries the unselected list
            partlists[0] = &editor.unselected_parts;
            break;
        case SEARCH_ALL:
            // No special action needed
            break;
    }

    // Skip empty lists
    auto& cur_list = partlists[partlist_idx];
    if (cur_list->empty()) {
        partlist_idx++;
        cur_list = partlists[partlist_idx];
        if (cur_list->empty()) {
            partlist_idx++;
        }
    }

    // Mark as done on creation if there's nothing left
    const u8 max_partlist_idx = search_type == SEARCH_ALL;
    if (partlist_idx > max_partlist_idx || cur_list->empty()) {
        done = true;
    }
}

const part_entry* const_part_iterator::next() noexcept {
    auto& cur_list = partlists[partlist_idx];
    const part_entry* part = &cur_list->operator[](part_idx);
    part_idx++;

    // Move on to the next list if needed
    if (part_idx >= cur_list->size()) {
        part_idx = 0;
        partlist_idx++;
        cur_list = partlists[partlist_idx];
    }

    const u8 max_partlist_idx = search_type == SEARCH_ALL;
    if (partlist_idx > max_partlist_idx || cur_list->empty()) {
        done = true;
    }

    return part;
}

static part_entry empty_part = {0};

part_entry* part_by_pos(editor_state& editor, vec3s8 target, partsearch_type search_hint) {
    const bool vacancy_result = cell_is_occupied(editor, target);
    const bool selection_result = cell_is_selected(editor, target);
    if (!vacancy_result && !selection_result) {
        // This cell isn't in the selection or vacancy grid, so there's no part here.
        empty_part = (part_entry){0};
        return &empty_part;
    }

    // Linearly search for the part
    part_iterator iter(editor, search_hint);
    while (!iter.done()) {
        part_entry* part = iter.next();

        // A part's max width is 8, so anything further away can't be a match
        if (abs(part->pos.x - target.x) > 8 || 
            abs(part->pos.y - target.y) > 8 || 
            abs(part->pos.z - target.z) > 8) {
            continue; // The part is too far away, skip it
        }

        // Loop over every cell this part occupies
        part_cell_iterator cell_iter(*part);
        while (!cell_iter.done) {
            // Get the next coordinate
            const vec3s8 cell = cell_iter.next();

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

bool vehicle_move_part(editor_state& editor, part_entry part, vec3s8 diff, vec3s8* adjust_out) {
    auto&& found_part = find_pod(editor.selected_parts, part);
    if (found_part == editor.selected_parts.end()) {
        // If for some reason we're moving an unselected part, handle that
        if (!contains(editor.unselected_parts, part)) {
            return false;
        }
    }
    part_entry& p = *found_part;

    bool needed_readjustment = false;
    // We loop over the 3 axes here
    for (u8 i = 0; i < 3; i++) {
        // Find position of this axis after the move
        const s8 new_pos = p.pos.raw[i] + diff.raw[i];
        if (new_pos >= VEH_MAX_DIM - 1) {
            // This part is at the border, there's nothing we can do.
            continue;
        }
        else if (new_pos >= 0) {
            // Everything's fine, update pos and move on
            p.pos.raw[i] += diff.raw[i];
            continue;
        }

        needed_readjustment = true;
        // Make this the new 0, update the output adjustment vector, and adjust
        // the rest of the parts
        p.pos.raw[i] = 0;
        if (adjust_out) {
            adjust_out->raw[i] = new_pos;
        }

        part_iterator iter(editor, SEARCH_ALL);
        while (!iter.done()) {
            
            // The part to be moved
            part_entry* other_part = iter.next();

            if (memcmp(&p, other_part, sizeof(part_entry)) == 0) {
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
