#include <malloc.h>

#include "render_garage.h"

typedef struct {
     vehicle* v;
}garage_state;

void* garage_init(vehicle* v) {
    garage_state* state = calloc(1, sizeof(*state));
    if (state == NULL) {
        LOG_MSG(info, "Couldn't allocate for renderer state!\n");
        return NULL;
    }
    state->v = v;

    return state;
}

void garage_render(void* ctx) {
    garage_state* state = (garage_state*)ctx;

}

void garage_destroy(void* ctx) {
    garage_state* state = (garage_state*)ctx;

}

renderer garage = {
    .init = garage_init,
    .render = garage_render,
    .destroy = garage_destroy,
};
