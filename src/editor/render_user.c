#include <string.h>
#include <stdatomic.h>

#include <GLFW/glfw3.h>

#include <common/utf8.h>
#include <common/logging.h>
#include <parts.h>
#include "editor.h"
#include "vehicle_edit.h"
#include "render_text.h"

// TODO: Move a bunch of this stuff into editor struct
char partname_buf[32] = "Large Folding Propeller"; // Longest part name
text_state part_name = {0};
text_state editing_mode = {0};
text_state camera_mode_text = {0};

// 500 characters because that's how many the text rendering shader can handle
char textbox_buf[500] = {0};
text_state textbox = {0};

// Use an atomic bool b/c I'm not sure if the callback is on the same thread
atomic_bool enable_textinput = false;
atomic_bool searchbuf_updated = false; // Set true on every codepoint callback

// This receives UTF-8 codepoint data from GLFW on every key press
void character_callback(GLFWwindow* window, unsigned int codepoint) {
    searchbuf_updated = true;
    if (enable_textinput) {
        const utf8 encoding = codepoint_to_utf8(codepoint);

        const u32 dest_len = strnlen(textbox_buf, sizeof(textbox_buf));
        const u32 utf8_len = strnlen(encoding.data, sizeof(encoding.data));
        // Only copy if the buffer has room
        if (dest_len < sizeof(textbox_buf) - utf8_len) {
            strncat(textbox_buf, encoding.data, utf8_len);
            text_update_transforms(&textbox);
        } else {
            LOG_MSG(debug, "Buffer full, not saving character %s\n", encoding.data);
        }
    }
    // TODO: Clone dmenu lol
}

enum {
    // Render this many results at a time
    PARTSEARCH_MENUSIZE = 7,
};

// Skip past this many matching entries before we start rendering (resets when
// target string changes, used to fake scrolling)
u32 partsearch_startoffset = 0;
text_state partsearch_results[PARTSEARCH_MENUSIZE];
const float partsearch_startheight = 0.5f;

void partsearch_update_render() {
    if (searchbuf_updated) {
        partsearch_startoffset = 0;
        u32 partsearch_menupos = 0; // Current offset in array of results
        for (u32 i = 0; i < NUM_PARTS; i++) {
            if (partsearch_menupos >= PARTSEARCH_MENUSIZE) {
                break;
            }

            const char* cur_part_name = partdata[i].name;
            if (cur_part_name == NULL) {
                continue;
            }
            if (strcasestr(cur_part_name, textbox_buf) != NULL) {
                text_state* result = &partsearch_results[partsearch_menupos++];
                result->text = cur_part_name;
                const float lineheight = text_get_lineheight(*result);
                // Subtract here because down is -Y
                result->pos[1] = partsearch_startheight - (lineheight * partsearch_menupos);
                text_update_transforms(result);
            }
        }

        // Make any remaining menu "slots" empty
        for (u32 i = partsearch_menupos; i < PARTSEARCH_MENUSIZE; i++) {
            text_state* result = &partsearch_results[partsearch_menupos++];
            result->text = "";
            text_update_transforms(result);
        }
        searchbuf_updated = false;
    }

    // Render
    for (u32 i = 0; i < PARTSEARCH_MENUSIZE; i++) {
        text_render(partsearch_results[i]);
    }

}

void ui_update_render(editor_state* editor) {
    // Setup on first run
    static bool initialized = false;
    if (!initialized) {
        part_name = text_render_prep(partname_buf, sizeof(partname_buf), text_default_scale, (vec2){-1.0f, -0.7f});
        editing_mode = text_render_prep(NULL, 32, text_default_scale, (vec2){-1.0f, 0.85f});
        camera_mode_text = text_render_prep(NULL, 32, text_default_scale, (vec2){-1.0f, 0.75f});
        textbox = text_render_prep(textbox_buf, sizeof(textbox_buf), text_default_scale, (vec2){-1.0f, 0.65f});
        for (u32 i = 0; i < ARRAY_SIZE(partsearch_results); i++) {
            // Y coord is set on the fly, we init to 0
            partsearch_results[i] = text_render_prep(NULL, 32, text_default_scale, (vec2){-1.0f, 0});
        }
        glfwSetCharCallback(editor->window, character_callback);
        initialized = true;
    }

    // Invalid cursor position forces the text to update on the first frame
    static vec3s16 last_selbox = {-1, -1, -1};

    // We can skip updating the part name if the cursor hasn't moved
    if (!vec3s16_eq(last_selbox, editor->sel_box)) {
        const vec3s8 pos = {editor->sel_box.x, editor->sel_box.y, editor->sel_box.z};
        const part_entry* part = part_by_pos(editor, pos, SEARCH_ALL);
        const part_info cur_part = part_get_info(part->id);
        strncpy(partname_buf, cur_part.name, sizeof(partname_buf));
        // Update part name & selection box
        text_update_transforms(&part_name);
        last_selbox = editor->sel_box;
    }

    static u8 last_edit_mode = MODE_ENUM_MAX;
    if (last_edit_mode != editor->mode) {
        switch (editor->mode) {
        case MODE_EDIT:
            editing_mode.text = "[Editing]";
            break;
        case MODE_MOVCAM:
            editing_mode.text = "[Freecam]";
            break;
        case MODE_MENU:
            editing_mode.text = "[Menu] (controls locked)";
            enable_textinput = true;
            break;
        default:
            editing_mode.text = "Unknown editor mode";
        }
        if (last_edit_mode == MODE_MENU) {
            // Disable when leaving menu mode
            enable_textinput = false;
            memset(textbox_buf, 0x00, sizeof(textbox_buf)); // Clear buffer
            text_update_transforms(&textbox);
            searchbuf_updated = true;
            partsearch_update_render();
        }

        text_update_transforms(&editing_mode);
        last_edit_mode = editor->mode;
    }
    static camera_mode last_cam_mode = CAMERA_MODE_ENUM_MAX;
    if (last_cam_mode != editor->cam.mode) {
        switch (editor->cam.mode) {
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
        last_cam_mode = editor->cam.mode;
    }

    // Handle backspace character because it's not sent to character callback
    if (input.backspace && !editor->prev_input.backspace) {
        textbox_buf[strlen(textbox_buf) - 1] = 0;
        text_update_transforms(&textbox);
        searchbuf_updated = true;
    }

    text_render(editing_mode);
    text_render(camera_mode_text);
    text_render(part_name);
    text_render(textbox);
    if (editor->mode == MODE_MENU) {
        partsearch_update_render();
    }
}

void ui_teardown() {
    text_free(part_name);
    text_free(editing_mode);
    text_free(camera_mode_text);
    text_free(textbox);
}

