#ifndef RENDER_GARAGE_H
#define RENDER_GARAGE_H
#include "gui_common.h"
#include "parts.h"

typedef struct {
    part_id id;
    model model;
}part_model;

// This is just a way to return an array without the compiler complaining
typedef struct {
    part_model models[NUM_PARTS + 1];
}garage_state;

garage_state garage_init(gui_state* gui);
void garage_render(garage_state* state, gui_state* gui);
void garage_destroy(garage_state* state);

#endif // RENDER_GARAGE_H
