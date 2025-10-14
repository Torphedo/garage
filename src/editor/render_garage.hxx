#pragma once

extern "C" {
#include <model.h>
#include <parts.h>
}

#include "editor.hxx"

typedef struct {
    part_id id;
    model m;
}part_model;

// State for rendering the garage "floor" and all of the vehicle parts.
struct garage_state : gui_layer {
    part_model models[NUM_PARTS + 1] = {};
    editor_state* editor = nullptr;

    explicit garage_state(editor_state* state) noexcept : editor(state) {

    }
    void init(GLFWwindow* window) noexcept override;
    void render(GLFWwindow* window) noexcept override;
    void destroy() noexcept override;
};
