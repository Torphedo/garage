#include <cstring>
#include <common/platform.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <shlwapi.h>
// They have a conflicting macro or enum name or something. Annoying.
#undef SEARCH_ALL
// strcasestr() is a glibc extension, on Windows we need to use their equivalent
const char* strcasestr(const char* a, const char* b) {
    // glibc's implementation considers empty strings to be a substring of any
    // other string, so this ensures we match that behaviour.
    if (strlen(a) == 0) {
        return b;
    }
    if (strlen(b) == 0) {
        return a;
    }
    // TODO: See if this works on UTF8? Not that it really matters in our use case.
    return strstr(a, b);
}
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include "render_text.hxx"
#include "vehicle_edit.hxx"

extern "C" {
#include <common/utf8.h>
#include <common/logging.h>

#include <primitives.h>
#include <parts.h>
#include "timing_targets.h"
}

// Use an atomic bool b/c I'm not sure if the callback is on the same thread.
// We can't put any of these globals in the editor struct b/c the callback
// can't access it.
bool enable_textinput = false;
bool searchbuf_updated = false; // Set true on every codepoint callback
// 200 characters because that's how many the text rendering shader can handle
char textbox_buf[200] = {0};

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
        } else {
            LOG_MSG(debug, "Buffer full, not saving character %s\n", encoding.data);
        }
    }
}

void draw_rect_panel(editor_state* editor, const mat4 quad_transform, const vec4 color) {
    glUseProgram(editor->vcolor_shader);
    glBindVertexArray(quad.vao);

    // Render panel
    glUniformMatrix4fv(editor->u_pvm, 1, GL_FALSE, (const float*)quad_transform);
    glUniform4fv(editor->u_paint, 1, (const float*)color);
    glDrawElements(GL_TRIANGLES, quad.idx_count, GL_UNSIGNED_SHORT, NULL);
}

void partsearch_update(editor_state* editor) {
    const double start_time = glfwGetTime();
    if ((input.enter && !editor->prev_input.enter) || (input.gp.a && !editor->prev_input.gp.a)) {
        editor->mode = MODE_EDIT;
        editor->sel_mode = SEL_ACTIVE;
        part_entry new_part = {
            .pos = {
                s8(CLAMP(0, editor->sel_box.x, INT8_MAX)),
                s8(CLAMP(0, editor->sel_box.y, INT8_MAX)),
                s8(CLAMP(0, editor->sel_box.z, INT8_MAX)),
            },
            .color = {255, 255, 255, 255},
        };

        bool failure = true;
        for (u32 i = 0; i < NUM_PARTS; i++) {
            const part_info info = partdata[i];
            const char* selected_item = editor->partsearch_results[editor->partsearch_selected_item].text;
            if (selected_item == NULL) {
                break;
            }
            if (strcmp(info.name, selected_item) == 0) {
                failure = false;
                new_part.id = info.id;
            }
        }
        if (!failure) {
            // Add the part
            list_add(&editor->selected_parts, &new_part);
            editor->v.part_count++;
            update_selectionmask(editor);
        }
    }

    // Up and down are kind of backwards. "Up" means increasing the menu index,
    // which is visually downwards.
    const bool up = (input.tab && !editor->prev_input.tab && !input.shift)  || editor->down_rising_edge() || (editor->move_y_rising_edge() == -1);
    const bool down = (input.tab && !editor->prev_input.tab && input.shift) || editor->up_rising_edge() || (editor->move_y_rising_edge() == 1);

    s8 diff = 0;
    if (up) {
        diff = 1;
    }
    if (down) {
        diff = -1;
    }
    // We have to check for W and S here because otherwise typing them would
    // move the selected item, which is unintuitive
    if (input.w || input.s) {
        diff = 0;
    }

    editor->partsearch_selected_item += diff;

    // Whether we needed to fake scroll this frame
    bool scrolled = false;
    if (editor->partsearch_selected_item == editor->partsearch_filled_slots || editor->partsearch_selected_item < 0) {
        // We're out of bounds, do fake scrolling and correct position
        editor->partsearch_startoffset += diff;
        editor->partsearch_selected_item -= diff;
        // Clamp to prevent going out of bounds
        editor->partsearch_startoffset = CLAMP(0, editor->partsearch_startoffset, NUM_PARTS);
        scrolled = true;
    }

    // Clamp so it doesn't go over the number of filled slots
    editor->partsearch_selected_item = CLAMP(0, editor->partsearch_selected_item, editor->partsearch_filled_slots - 1);

    // Update search results when the contents of the search box change
    const float partsearch_startheight = 0.4f;
    if (searchbuf_updated) {
        // Reset our position & fake scrolling when search buffer changes
        editor->partsearch_startoffset = 0;
        editor->partsearch_selected_item = 0;
    }

    // Redo our search
    if (searchbuf_updated || scrolled) {
        // Update textbox display
        text_update_transforms(&editor->textbox);
        u32 menupos = 0; // Current offset in array of results
        u32 hit_count = 0; // How many matches we've gotten.
        for (u32 i = 0; i < NUM_PARTS; i++) {
            if (menupos >= PARTSEARCH_MENUSIZE) {
                // Stop searching when we fill all of our result slots
                break;
            }

            const char* cur_part_name = partdata[i].name;
            if (cur_part_name == NULL) {
                // This can happen for parts 
                continue;
            }
            if (strcasestr(cur_part_name, textbox_buf) != NULL) {
                hit_count++;
                if (hit_count <= editor->partsearch_startoffset) {
                    // This match should be scrolled out of sight, skip it.
                    continue;
                }

                text_state* result = &editor->partsearch_results[menupos++];
                result->text = cur_part_name;
                const float lineheight = text_get_lineheight(*result);
                // Subtract here because down is -Y
                result->pos[1] = partsearch_startheight - (lineheight * menupos);
                text_update_transforms(result);
            }
        }
        editor->partsearch_filled_slots = menupos; // Save how many slots we filled

        // Make any remaining menu "slots" empty
        for (u32 i = menupos; i < PARTSEARCH_MENUSIZE; i++) {
            text_state* result = &editor->partsearch_results[i];
            result->text = "";
            text_update_transforms(result);
        }
        searchbuf_updated = false;
    }

    DBG_ASSERT_PERF(start_time, 1.2);
}

void partsearch_render(editor_state* editor) {
    const double start_time = glfwGetTime();
    const float partsearch_startheight = 0.4f;
    // Setup quad transform
    mat4 mdl = {0};
    glm_mat4_identity(mdl);
    // Scale to screen space. This messes up our coordinates, so the
    // coordinates ahead aren't screen-space coords.
    glm_scale_uni(mdl, 1.0f / QUAD_SIZE);
    glm_rotate_x(mdl, glm_rad(90.0f), mdl); // Face quad towards camera
    glm_translate(mdl, (vec3){0.0f, 0.20f, 3.5f});
    glm_scale(mdl, (vec3){0.55f, 1.0f, 0.75f}); // Scale to reasonable size

    const vec4 outer_color = {0.95f, 0.95f, 0.95f, 1.0f};
    draw_rect_panel(editor, mdl, outer_color);

    {
        // Sorry, this is a stupid hack where I just pass it the default scale.
        // We should make it just take a scale value.
        const float lineheight = text_get_lineheight((text_state){.scale = text_default_scale});
        // Subtract here because down is -Y
        const float ypos = partsearch_startheight - (lineheight * (editor->partsearch_selected_item + 1));

        // Setup transform for highlight box
        // Sorry for all the magic numbers, they just come from trial & error.
        // We're redoing the work above so we can use real screenspace coords
        // and perfectly align the box to the screenspace coords of the text.
        mat4 highlight_trans = {0};
        glm_mat4_identity(highlight_trans);
        glm_translate(highlight_trans, (vec3){0.0f, ypos - 0.06f, 0.00f});
        glm_scale_uni(highlight_trans, 1.0f / QUAD_SIZE);
        glm_scale(highlight_trans, (vec3){0.55f * 0.97f, lineheight * 0.5f, 1});
        glm_rotate_x(highlight_trans, glm_rad(90.0f), highlight_trans); // Face quad towards camera

        // Render highlight bar
        draw_rect_panel(editor, highlight_trans, (vec4){0, 0, 0, 1});
    }

    // Render inner panel
    glm_scale(mdl, (vec3){0.97f, 1.0f, 0.95f}); // Scale to a smaller box
    glm_translate(mdl, (vec3){0.00f, -0.03f, 0.00f}); // Move in front of the other quad
    const vec4 inner_color = {0.2f, 0.2f, 1.0f, 1.0f};
    draw_rect_panel(editor, mdl, inner_color);

    // Render search results
    for (u32 i = 0; i < PARTSEARCH_MENUSIZE; i++) {
        text_render(editor->partsearch_results[i]);
    }

    DBG_ASSERT_PERF(start_time, 1.2);
}

void ui_update_render(editor_state* editor) {
    const double time_start = glfwGetTime();
    // Setup on first run
    static bool initialized = false;
    if (!initialized) {
        editor->part_name = text_render_prep(editor->partname_buf, sizeof(editor->partname_buf), text_default_scale, (vec2){-1.0f, -0.7f});
        editor->editing_mode = text_render_prep(NULL, 32, text_default_scale, (vec2){-1.0f, 0.85f});
        editor->camera_mode_text = text_render_prep(NULL, 32, text_default_scale, (vec2){-1.0f, 0.75f});
        editor->textbox = text_render_prep(textbox_buf, sizeof(textbox_buf), text_default_scale, (vec2){-0.5f, 0.55f});
        for (u32 i = 0; i < ARRAY_SIZE(editor->partsearch_results); i++) {
            // Y coord is set on the fly, we init to 0
            text_state* result = &editor->partsearch_results[i];
            *result = text_render_prep(NULL, 32, text_default_scale, (vec2){-0.5f, 0});
        }
        glfwSetCharCallback(editor->window, character_callback);
        initialized = true;
        searchbuf_updated = true;
    }

    // Invalid cursor position forces the text to update on the first frame
    static vec3s16 last_selbox = {-1, -1, -1};

    // We can skip updating the part name if the cursor hasn't moved
    if (!vec3s16_eq(last_selbox, editor->sel_box)) {
        const vec3s8 pos = {s8(editor->sel_box.x), s8(editor->sel_box.y), s8(editor->sel_box.z)};
        const part_entry* part = part_by_pos(editor, pos, SEARCH_ALL);
        const part_info cur_part = part_get_info((part_id)part->id);
        strncpy(editor->partname_buf, cur_part.name, sizeof(editor->partname_buf));
        // Update part name & selection box
        text_update_transforms(&editor->part_name);
        last_selbox = editor->sel_box;
    }

    static u8 last_edit_mode = MODE_ENUM_MAX;
    if (last_edit_mode != editor->mode) {
        switch (editor->mode) {
        case MODE_EDIT:
            editor->editing_mode.text = "[Editing]";
            break;
        case MODE_MOVCAM:
            editor->editing_mode.text = "[Freecam]";
            break;
        case MODE_MENU:
            editor->editing_mode.text = "[Menu] (controls locked)";
            enable_textinput = true;
            break;
        default:
            editor->editing_mode.text = "Unknown editor mode";
        }
        if (last_edit_mode == MODE_MENU) {
            // Disable when leaving menu mode
            enable_textinput = false;
            memset(textbox_buf, 0x00, sizeof(textbox_buf)); // Clear buffer
            text_update_transforms(&editor->textbox);

            // Also clear part search menu
            searchbuf_updated = true;
        }

        text_update_transforms(&editor->editing_mode);
        last_edit_mode = editor->mode;
    }
    static camera_mode last_cam_mode = CAMERA_MODE_ENUM_MAX;
    if (last_cam_mode != editor->cam.mode) {
        switch (editor->cam.mode) {
        case CAMERA_ORBIT:
            editor->camera_mode_text.text = "CAMSTYLE: Orbit";
            break;
        case CAMERA_POV:
            editor->camera_mode_text.text = "CAMSTYLE: Minecraft";
            break;
        case CAMERA_FLY:
            editor->camera_mode_text.text = "CAMSTYLE: Flying";
            break;
        default:
            editor->camera_mode_text.text = "CAMSTYLE: Unknown";
        }

        text_update_transforms(&editor->camera_mode_text);
        last_cam_mode = editor->cam.mode;
    }

    // Handle backspace character because it's not sent to character callback
    if (input.backspace && !editor->prev_input.backspace) {
        textbox_buf[strlen(textbox_buf) - 1] = 0;
        text_update_transforms(&editor->textbox);
        searchbuf_updated = true;
    }

    if (editor->mode == MODE_MENU) {
        partsearch_update(editor);
        partsearch_render(editor);
        text_render(editor->textbox);
    }
    else {
        text_render(editor->part_name);
    }

    text_render(editor->editing_mode);
    text_render(editor->camera_mode_text);

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    DBG_ASSERT_PERF(time_start, 1.2);
}

void ui_teardown(editor_state* editor) {
    text_free(editor->part_name);
    text_free(editor->editing_mode);
    text_free(editor->camera_mode_text);
    text_free(editor->textbox);

    for (u32 i = 0; i < ARRAY_SIZE(editor->partsearch_results); i++) {
        text_free(editor->partsearch_results[i]);
    }
}
