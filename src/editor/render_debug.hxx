#pragma once
#include "editor.hxx"

// This file was originally for debug rendering, but the collision and
// selection boxes ended up becoming part of the user-facing UI. So now it's
// just for those.

struct layer_debug : gui_layer {
    const editor_state* editor = nullptr;

    explicit layer_debug(const editor_state* editor) noexcept : editor(editor) {

    }
    void render(GLFWwindow* window) noexcept override;
};
