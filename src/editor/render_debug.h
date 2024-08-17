#ifndef RENDER_DEBUG_H
#define RENDER_DEBUG_H
#include "gui_common.h"

// This file was originally for debug rendering, but the collision and
// selection boxes ended up becoming part of the user-facing UI. So now it's
// just for those.

void debug_render(gui_state* gui);

#endif // RENDER_DEBUG_H
