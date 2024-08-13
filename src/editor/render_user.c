#include <string.h>

#include "../parts.h"
#include "gui_common.h"
#include "render_text.h"

char partname_buf[32] = "Large Folding Propeller";
text_state part_name = {0};
text_state editing_mode = {0};
text_state camera_mode_text = {0};

void ui_update_render(gui_state* gui) {
    // Setup on first run
    static bool initialized = false;
    if (!initialized) {
        part_name = text_render_prep(partname_buf, sizeof(partname_buf), 0.03f, (vec2){-1, -0.7f});
        editing_mode = text_render_prep(NULL, 32, 0.03f, (vec2){-1.01f, 0.85f});
        camera_mode_text = text_render_prep(NULL, 32, 0.03f, (vec2){-1.00f, 0.75f});
        initialized = true;
    }

    // Invalid cursor position forces the text to update on the first frame
    static vec3s16 last_selbox = {-1, -1, -1};

    // We can skip updating the part name if the cursor hasn't moved
    if (!vec3s16_eq(last_selbox, gui->sel_box)) {
        part_entry* p = part_by_pos(gui->v, (vec3s8){gui->sel_box.x, gui->sel_box.y, gui->sel_box.z});
        part_info cur_part = part_get_info(p->id);
        strncpy(partname_buf, cur_part.name, sizeof(partname_buf));
        // Update part name & selection box
        text_update_transforms(&part_name);
        last_selbox = gui->sel_box;
    }

    static u8 last_edit_mode = MODE_ENUM_MAX;
    if (last_edit_mode != gui->mode) {
        switch (gui->mode) {
        case MODE_EDIT:
            editing_mode.text = "[Editing]";
            break;
        case MODE_MOVCAM:
            editing_mode.text = "[Freecam]";
            break;
        case MODE_MENU:
            editing_mode.text = "[Menu] (controls locked)";
            break;
        default:
            editing_mode.text = "Unknown editor mode";
        }

        text_update_transforms(&editing_mode);
        last_edit_mode = gui->mode;
    }
    static camera_mode last_cam_mode = CAMERA_MODE_ENUM_MAX;
    if (last_cam_mode != gui->cam.mode) {
        switch (gui->cam.mode) {
        case CAMERA_ORBIT:
            camera_mode_text.text = "CAMSTYLE: Orbit";
            break;
        case CAMERA_POV:
            camera_mode_text.text = "CAMSTYLE: Minecraft";
            break;
        case CAMERA_FLY:
            camera_mode_text.text = "CAMSTYLE: Flying";
            break;
        default:
            camera_mode_text.text = "CAMSTYLE: Unknown";
        }

        text_update_transforms(&camera_mode_text);
        last_cam_mode = gui->cam.mode;
    }

    text_render(editing_mode);
    text_render(camera_mode_text);
    text_render(part_name);
}

void ui_teardown() {
    text_free(part_name);
}

