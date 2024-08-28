#ifndef VEHICLE_EDIT_H
#define VEHICLE_EDIT_H
// This file is for all vehicle-related code specific to this editor. I
// separated it out so vehicle.c/h can be used without any other editor code.

#include <stdbool.h>

#include <cglm/cglm.h>

#include <common/int.h>
#include <common/vector.h>
#include <vehicle.h>
#include <parts.h>
#include "editor.h"

typedef enum {
    SEARCH_SELECTED,
    SEARCH_UNSELECTED,
    SEARCH_ALL,
}partsearch_type;

typedef struct {
    part_info info;
    part_entry part;
    bool done;
}part_cell_iterator;

typedef struct {
    list partlists[2];
    partsearch_type search_type;
    u8 partlist_idx; // Current index into the list array
    u32 part_idx; // Current index into the current list
    bool done;
}part_iterator;

// Safely get & set values from vehicle bitmask (with bounds checking)
bool vehiclemask_get_3d(vehicle_bitmask* mask, vec3s8 cell);
void vehiclemask_set_3d(vehicle_bitmask* mask, vec3s8 cell, u8 val);

bool cell_is_selected(editor_state* editor, vec3s8 cell);

// Uses part data to find the centerpoint of a vehicle.
// (returns float vector for convenience, since centerpoint could be a decimal)
vec3s vehicle_find_center(const editor_state* editor, partsearch_type search_type);

// Rotate all selected parts about their centerpoint. Forward & side diff
// represent user inputs on a joystick/D-Pad/keyboard X/Y axes.
// Returns whether the rest of the vehicle was adjusted (like vehicle_move_part())
bool vehicle_rotate_selection(editor_state* editor, s8 forward_diff, s8 side_diff, s8 roll_diff);

// Check if the selected parts overlap with the rest of the vehicle
bool vehicle_selection_overlap(editor_state* editor);

// Wipe & reconstruct individual 3d grids from scratch
void update_selectionmask(editor_state* editor);
void update_vacancymask(editor_state* editor);

// Setup an iterator from a part entry. Returns an iteration context.
// There's no need to free the iteration context.
part_cell_iterator part_cell_iterator_setup(part_entry p);

// Get the next item and advance.
vec3s8 part_cell_iterator_next(part_cell_iterator* ctx);

part_iterator part_iterator_setup(editor_state editor, partsearch_type search_type);
part_entry* part_iterator_next(part_iterator* ctx);

// Look up a part by position.
// Use the enum to search only one list, or tell it to search both. 
// When searching both lists, the selected list is prioritized.
// Returns an all-zero part on failure.
part_entry* part_by_pos(editor_state* editor, vec3s8 target, partsearch_type search_hint);

// Move a part by a 3D vector. If the new position is out of bounds (< 0), that
// position will be the new zero and the other parts are adjusted accordingly.
// If the vehicle was adjusted, writes to an output vector to indicate the
// vector of the adjustment. This output vector can be NULL.
// Returns a boolean indicating if the vehicle had to be adjusted.
bool vehicle_move_part(editor_state* editor, part_entry part, vec3s16 diff, vec3s16* adjust_out);

#endif // VEHICLE_EDIT_H

