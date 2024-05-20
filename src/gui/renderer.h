#ifndef RENDERER_H
#define RENDERER_H
#include <common/int.h>
#include <vehicle.h>

// Standard interface to facilitate multiple renderers per frame. For example, a
// UI renderer would render on top of the scene renderer and have separate code.
// This also allows multiple copies of a renderer at once.
typedef struct {
    // For initial setup, like compiling shaders or loading assets. Pass the
    // returned pointer to render() and destroy(), they use it to store data.
    void* (* init)(vehicle* v);

    // For taking over the rendering pipeline (binding shaders, etc.), then
    // rendering on top and updating internal state (checking for input, etc.)
    void (*render)(void* ctx);

    // Free all buffers, unload all shaders, etc.
    void (*destroy)(void* ctx);
}renderer;

#endif // RENDERER_H
