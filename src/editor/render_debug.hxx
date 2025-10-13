#pragma once
#include "editor.hxx"

// This file was originally for debug rendering, but the collision and
// selection boxes ended up becoming part of the user-facing UI. So now it's
// just for those.

void debug_render(editor_state* editor);
