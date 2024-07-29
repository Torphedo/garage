#ifndef RENDER_TEXT_H
#define RENDER_TEXT_H
#include <stdbool.h>

#include <cglm/cglm.h>

#include <common/int.h>

// All the information needed to render a text buffer (Please treat as opaque)
typedef struct {
    mat4* transforms; // 3D transform of each character's quad
    vec4* texcoords;  // Top-left & bottom right UVs of each character's quad
    u32 num_chars;
    float scale; // Text scale
    vec2 pos;    // X/Y pos of the first character
    char* text;
}text_state;

// Call these at program startup/shutdown.

// Loads font & renders to a compressed texture, then sets up OpenGL buffers.
bool text_renderer_setup(const char* ttf_path);
void text_renderer_cleanup();

// Call these to render individual strings or text buffers.

// Allocates data needed to render the string. Automatically calls
// text_update_transforms(), so you can call text_render() right after.
//
// [len] is an optional hint of how many characters the text buffer can contain
// (useful for stack buffers). Passing 0 falls back to strlen().
//
// [text] is the text buffer used to update the transforms.
// You can pass in a NULL pointer, as long as you give an allocation hint.
// (but this means text_update_transforms() won't be called)
//
// [scale] is the font size, & [pos] is the 2D position of the first character
text_state text_render_prep(const char* text, u32 len, float scale, vec2 pos);

// Uses the UTF-8 in the text pointer to update the transforms and texture
// coordinates for each character. Call this if the pointer or string contents
// changed since you last rendered.
// Safely fails with a warning in the console if it finds a NULL pointer.
void text_update_transforms(text_state* ctx);

// Renders the text as it was the last time you called text_update_transforms()
// The text pointer is unused and not stored anywhere but the context struct.
// If you have a static string, you might find it convenient to free your text
// buffer, set it to NULL in the struct, then keep rendering it.
void text_render(text_state ctx);

// Frees all internal buffers in [ctx]
// (text pointer isn't freed, because it could be on the stack)
void text_free(text_state ctx);

#endif // RENDER_TEXT_H

