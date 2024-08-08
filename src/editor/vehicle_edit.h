#ifndef VEHICLE_EDIT_H
#define VEHICLE_EDIT_H
#include <stdbool.h>

#include <vehicle.h>
#include <common/int.h>

// This file is for all vehicle-related code specific to this editor. I
// separated it out so vehicle.c/h can be used without any other editor code.

// Used to store a compact 3D grid of parts at 1 bit per cell.
// The editor uses 2 of these, one for storing which cells are occupied and one
// for storing which cells are selected.
typedef u8 vehicle_bitmask[VEH_MAX_DIM][VEH_MAX_DIM][VEH_MASK_BYTE_WIDTH];

// Safely get & set values from vehicle bitmask (with bounds checking)
bool vehiclemask_get_3d(vehicle_bitmask* mask, s8 x, s8 y, s8 z);
void vehiclemask_set_3d(vehicle_bitmask* mask, s8 x, s8 y, s8 z, u8 val);

// Rotate all selected parts about their centerpoint. Forward & side diff
// represent user inputs on a joystick/D-Pad/keyboard X/Y axes.
// [cam_view] is the vector of the camera view obtained from camera_facing().
bool vehicle_rotate_selection(vehicle* v, vehicle_bitmask* selection, vehicle_bitmask* vacancy, s8 forward_diff, s8 side_diff, vec3s cam_view);

// Check if the selected parts overlap with the rest of the vehicle
bool vehicle_selection_overlap(vehicle* v, vehicle_bitmask* mask);

// Update the vacancy & selection masks based on the current part data
void update_vehiclemask(vehicle* v, vehicle_bitmask* vacancy, vehicle_bitmask* selection);

#endif // VEHICLE_EDIT_H

