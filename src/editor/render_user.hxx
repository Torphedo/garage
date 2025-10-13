#pragma once

extern "C" {
#include "editor.hxx"
}

// This file is for rendering the user interface

void ui_update_render(editor_state* editor);
void ui_teardown(editor_state* editor);
