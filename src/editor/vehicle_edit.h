#ifndef VEHICLE_EDIT_H
#define VEHICLE_EDIT_H
// This file is for all vehicle-related code specific to this editor. I
// separated it out so vehicle.c/h can be used without any other editor code.

#include <stdbool.h>

#include <cglm/cglm.h>

#include <common/int.h>
#include <vehicle.h>
#include <parts.h>
#include "editor.h"

// Safely get & set values from vehicle bitmask (with bounds checking)
bool vehiclemask_get_3d(vehicle_bitmask* mask, s8 x, s8 y, s8 z);
void vehiclemask_set_3d(vehicle_bitmask* mask, s8 x, s8 y, s8 z, u8 val);

bool cell_is_selected(editor_state* editor, vec3s8 cell);

vec3s16 vehicle_selection_center(editor_state* editor);

// Rotate all selected parts about their centerpoint. Forward & side diff
// represent user inputs on a joystick/D-Pad/keyboard X/Y axes.
bool vehicle_rotate_selection(editor_state* editor, s8 forward_diff, s8 side_diff, s8 roll_diff);

// Check if the selected parts overlap with the rest of the vehicle
bool vehicle_selection_overlap(editor_state* editor);

// Update the vacancy & selection masks based on the current part data
void update_vehiclemask(vehicle* v, list selected_parts, vehicle_bitmask* vacancy, vehicle_bitmask* selection);

typedef struct {
    part_info info;
    part_entry part;
    bool done;
}part_cell_iterator;

// Setup an iterator from a part entry. Returns an iteration context.
// There's no need to free the iteration context.
part_cell_iterator part_cell_iterator_setup(part_entry p);

// Get the next item and advance.
vec3s8 part_cell_iterator_next(part_cell_iterator* ctx);

// Look up a part by its origin position.
// This will return an index into the part array, or -1 on error.
s32 part_by_pos(vehicle* v, vec3s8 target);

#endif // VEHICLE_EDIT_H

