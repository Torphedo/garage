#include <string.h>

#include "parts.h"
#include "gui_common.h"
#include "render_text.h"
#include "vehicle.h"

char partname_buf[32] = "Large Folding Propeller";
text_state part_name = {0};

void ui_update_render(gui_state* gui) {
    // Setup on first run
    static bool initialized = false;
    if (!initialized) {
        part_name = text_render_prep(partname_buf, sizeof(partname_buf), 0.03f, (vec2){-1, -0.7f});
        initialized = true;
    }

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
        // Update part name & selection box
        text_update_transforms(&part_name);
        last_selbox = gui->sel_box;
    }

    text_render(part_name);
}

void ui_teardown() {
    text_free(part_name);
}

