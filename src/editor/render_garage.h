#ifndef RENDER_GARAGE_H
#define RENDER_GARAGE_H
#include <model.h>
#include "editor.h"
#include <parts.h>

typedef struct {
    part_id id;
    model model;
}part_model;

// State for rendering the garage "floor" and all of the vehicle parts.
typedef struct {
    part_model models[NUM_PARTS + 1];
    // This is just a way to return an array without the compiler complaining
}garage_state;

// This used to be an interface implementation with function pointers and a
// context pointer, but I decided to make it informal (but consistent) and
// avoid those heap allocations. - Torph

// Setup part models needed to render a vehicle
garage_state garage_init(editor_state* editor);
void garage_render(garage_state* state, editor_state* editor);
void garage_destroy(garage_state* state);

#endif // RENDER_GARAGE_H
