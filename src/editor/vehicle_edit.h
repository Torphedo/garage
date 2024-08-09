#ifndef VEHICLE_EDIT_H
#define VEHICLE_EDIT_H
#include <stdbool.h>

#include "gui_common.h"
#include <vehicle.h>
#include <common/int.h>

// This file is for all vehicle-related code specific to this editor. I
// separated it out so vehicle.c/h can be used without any other editor code.

// Safely get & set values from vehicle bitmask (with bounds checking)
bool vehiclemask_get_3d(vehicle_bitmask* mask, s8 x, s8 y, s8 z);
void vehiclemask_set_3d(vehicle_bitmask* mask, s8 x, s8 y, s8 z, u8 val);

vec3s8 vehicle_selection_center(vehicle* v);

// Rotate all selected parts about their centerpoint. Forward & side diff
// represent user inputs on a joystick/D-Pad/keyboard X/Y axes.
bool vehicle_rotate_selection(gui_state* gui, s8 forward_diff, s8 side_diff, s8 roll_diff);

// Check if the selected parts overlap with the rest of the vehicle
bool vehicle_selection_overlap(vehicle* v, vehicle_bitmask* mask);

// Update the vacancy & selection masks based on the current part data
void update_vehiclemask(vehicle* v, vehicle_bitmask* vacancy, vehicle_bitmask* selection);

#endif // VEHICLE_EDIT_H

