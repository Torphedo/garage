#pragma once
// This file is for all vehicle-related code specific to this editor. I
// separated it out so vehicle.c/h can be used without any other editor code.

#include <cglm/cglm.h>

#include <common/int.h>

extern "C" {
#include <vector.h>
#include <vehicle.h>
#include <parts.h>
}

#include "layer_editor.hxx"

typedef enum {
    SEARCH_SELECTED,
    SEARCH_UNSELECTED,
    SEARCH_ALL,
}partsearch_type;

struct part_cell_iterator {
    part_info info = {};
    part_entry part = {};
    bool done = false;
    u32 cell_idx = 0;

    explicit part_cell_iterator(part_entry p) : part(p), info(part_get_info((part_id)p.id)) {

    }

    // Get the next item and advance.
    vec3s8 next();
};

struct const_part_iterator {
    const std::vector<part_entry>* partlists[2];
    partsearch_type search_type = SEARCH_SELECTED;
    u8 partlist_idx = 0; // Current index into the list array
    u32 part_idx = 0; // Current index into the current list
    bool done = false;

    const_part_iterator(const editor_state& editor, partsearch_type search_type) noexcept;

    const part_entry* next() noexcept;
};

struct part_iterator {
    const_part_iterator iter;

    part_iterator(editor_state& editor, partsearch_type search_type) noexcept : iter(editor, search_type) {
        return;
    }

    part_entry* next() noexcept {
        return const_cast<part_entry*>(iter.next());
    }

    bool done() {
        return iter.done;
    }
};

// Safely get & set values from vehicle bitmask (with bounds checking)
bool vehiclemask_get_3d(const vehicle_bitmask* mask, vec3s8 cell);
void vehiclemask_set_3d(vehicle_bitmask* mask, vec3s8 cell, u8 val);

bool cell_is_selected(const editor_state& editor, vec3s8 target);

// Uses part data to find the centerpoint of a vehicle.
// (returns float vector for convenience, since centerpoint could be a decimal)
vec3s vehicle_find_center(const editor_state* editor, partsearch_type search_type);

// Rotate all selected parts about their centerpoint. Forward & side diff
// represent user inputs on a joystick/D-Pad/keyboard X/Y axes.
// Returns whether the rest of the vehicle was adjusted (like vehicle_move_part())
bool vehicle_rotate_selection(editor_state* editor, s8 forward_diff, s8 side_diff, s8 roll_diff);

// Check if the selected parts overlap with the rest of the vehicle
bool vehicle_selection_overlap(const editor_state& editor);

// Wipe & reconstruct individual 3d grids from scratch
void update_selectionmask(editor_state& editor);
void update_vacancymask(editor_state& editor);

// Look up a part by position.
// Use the enum to search only one list, or tell it to search both.
// When searching both lists, the selected list is prioritized.
// Returns an all-zero part on failure.
part_entry* part_by_pos(editor_state& editor, vec3s8 target, partsearch_type search_hint);

/// @brief Move a part by a 3D vector.
///
/// If the new position is out of bounds (< 0), that position will be the new
/// zero and the other parts are adjusted accordingly.
/// @param editor The editor instance to edit
/// @param part The part to move
/// @param diff How to move the part
/// @param adjust_out Receives the amount that all other parts had to be
///                   adjusted to keep all coordinates positive. May be NULL.
/// @return Whether the other parts had to be adjusted to keep all the part
///         coordinates positive.
bool vehicle_move_part(editor_state& editor, part_entry part, vec3s8 diff, vec3s8* adjust_out);
