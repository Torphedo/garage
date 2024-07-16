#include <glad/glad.h>
#include <string.h>

#include <common/logging.h>
#include "parts.h"
#include "primitives.h"
#include "camera.h"
#include "gui_common.h"
#include "render_text.h"
#include "vehicle.h"

typedef struct {
    gui_state* gui;
    text_state part_name;
}ui_render_state;

char partname_buf[32] = "Large Folding Propeller";

void* ui_init(gui_state* gui) {
    ui_render_state* state = calloc(1, sizeof(*state));
    if (state == NULL) {
        LOG_MSG(error, "Failed to allocate state struct\n");
        return NULL;
    }
    state->gui = gui;
    state->part_name = text_render_prep(partname_buf, sizeof(partname_buf), 0.03f, (vec2){-1, -0.7f});

    return state;
}

void ui_update(void* ctx) {
    ui_render_state* state = (ui_render_state*)ctx;
    gui_state* gui = (gui_state*) state->gui;

    // Invalid cursor position forces the text to update on the first frame
    static vec3s16 last_selbox = {-1, -1, -1};

    // We can skip updating the part name if the cursor hasn't moved
    if (!vec3s16_eq(last_selbox, gui->sel_box)) {
        part_entry* p = part_by_pos(gui->v, (vec3s8){gui->sel_box.x, gui->sel_box.y, gui->sel_box.z});
        if (p == NULL) {
            // No part here, blank out the rendered name.
            memset(partname_buf, 0x00, sizeof(partname_buf));
        }
        else {
            part_info cur_part = part_get_info(p->id);
            strncpy(partname_buf, cur_part.name, sizeof(partname_buf));
        }
        // Update part name
        text_update_transforms(&state->part_name);
    }
    last_selbox = gui->sel_box;

    text_render(state->part_name);
}

void ui_destroy(void* ctx) {
    free(ctx);
}

typedef void (*renderproc)(void*);
renderer ui = {
    ui_init,
    (renderproc)ui_update,
    (renderproc)ui_destroy,
};

