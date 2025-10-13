#pragma once
#include <cglm/cglm.h>

#include <common/int.h>

// All the information needed to render a text buffer
// (Please treat the transforms & texcoords as opaque)
typedef struct {
    mat4* transforms; // 3D transform of each character's quad
    vec4* texcoords;  // Top-left & bottom right UVs of each character's quad
    u32 num_chars;
    float scale; // Text scale
    // TODO: Make this a vec2s so it's assignable
    vec2 pos;    // X/Y pos of the first character
    const char* text;  // UTF-8 text data, or NULL
}text_state;

static const float text_default_scale = 0.03f;

// Call these at program startup/shutdown.

// Loads font & renders to a compressed texture, then sets up OpenGL buffers.
bool text_renderer_setup(const char* ttf_path);
void text_renderer_cleanup();

// Call these to render individual strings or text buffers.

/// @brief Sets up data needed to render the string.
///
/// Automatically calls text_update_transforms(), so you can call text_render() right after.
/// @param len Optional hint of how many characters the text buffer can contain
/// (useful for stack buffers). Pass 0 to fall back to strlen().
///
/// @param text The text buffer used to update the transforms. Can be NULL, as
/// long as you give an allocation hint. (but this means
/// text_update_transforms() won't be called)
///
/// @param scale The font size
/// @param pos The 2D screen position of the first character
text_state text_render_prep(const char* text, u32 len, float scale, vec2 pos);

float text_get_lineheight(text_state t);

/// @brief Updates rendering state to match the current text state
///
/// Text can be any UTF-8. Call this if the pointer or string contents changed
/// since you last rendered. Safely fails with a warning in the console if it
/// finds a NULL pointer.
void text_update_transforms(text_state* ctx);

/// @brief Renders the text as it was the last time you called text_update_transforms()
///
/// The text pointer may be free/invalid or NULL without impacting rendering.
/// If you have a static string, you might find it convenient to free your text
/// buffer, set it to NULL in the struct, then keep rendering it.
void text_render(text_state ctx);

// Frees all internal buffers in [ctx]
// Caller is responsible for freeing the text buffer if necessary.
void text_free(text_state ctx);
