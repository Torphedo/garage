#pragma once

extern "C" {
#include <model.h>
#include "editor.h"
#include <parts.h>
}
#include "layer.hxx"

typedef struct {
    part_id id;
    model m;
}part_model;

// State for rendering the garage "floor" and all of the vehicle parts.
struct garage_state : gui_layer {
    part_model models[NUM_PARTS + 1] = {};
    editor_state* editor = nullptr;

    void init(editor_state* state) noexcept;
    void render() noexcept;
    void destroy() noexcept override;
};
