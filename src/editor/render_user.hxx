#pragma once

#include "editor.hxx"

enum {
    // Render this many part search results at a time
    PARTSEARCH_MENUSIZE = 10,
};

// This file is for rendering the user interface
struct editor_ui {
    text_state part_name = {};
    char partname_buf[32] = {}; // Backing text buffer, enough for longest part name
    text_state editing_mode = {};
    text_state camera_mode_text = {};
    bool initialized = false;

    // State for the part search menu
    text_state textbox = {}; // User input search box
    text_state partsearch_results[PARTSEARCH_MENUSIZE] = {};
    // Skip past this many matching entries before we start rendering (resets when
    // target string changes, used to fake scrolling)
    s32 partsearch_startoffset = {};
    s8 partsearch_selected_item = {}; // Index of selected search result
    u32 partsearch_filled_slots = {}; // Number of non-empty search result slots

    explicit editor_ui(GLFWwindow* window) noexcept;
    void update_render(editor_state* editor) noexcept;
    void teardown() noexcept;

    void partsearch_update(editor_state* editor);
    void partsearch_render(editor_state* editor);
};
