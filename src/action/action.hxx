#pragma once
#include <map>
#include <string>
#include <common/int.h>
#include "input.hxx"
#include "keys.hxx"

namespace action {
    extern input_t input;
    extern input_t prev_input;

    struct bind {
        keyboard_key pc = KEY_UNKNOWN;
        mouse_button mouse = MOUSE_UNKNOWN;
        gamepad_button gamepad = GAMEPAD_BUTTON_UNKNOWN;
        bool rising_edge(const input_t& cur_input = action::input, const input_t& last_input = action::prev_input) const noexcept;
        bool falling_edge(const input_t& cur_input = action::input, const input_t& last_input = action::prev_input) const noexcept;

        bind(keyboard_key key, gamepad_button gp) : pc(key), gamepad(gp) { }
        explicit bind(keyboard_key key) : pc(key) { }
        explicit bind(gamepad_button gp) : gamepad(gp) { }
        bind() = default;
    };

    // An entry defining a single gameplay action
    struct entry {
        bind main;
        bind alt;

        bool rising_edge(const input_t& cur_input = action::input, const input_t& last_input = action::prev_input) const noexcept;
        bool falling_edge(const input_t& cur_input = action::input, const input_t& last_input = action::prev_input) const noexcept;

        entry(bind b) : main(b) { }
        explicit entry(keyboard_key k) : main(bind(k)) { }
        explicit entry(gamepad_button b) : main(bind(b)) { }
        entry() = default;
    };

    typedef std::map<std::basic_string<char>, entry> map;
} // namespace action
