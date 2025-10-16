#include "action.hxx"
#include "action/keys.hxx"

namespace action {

bool bind::rising_edge(const input_t& cur_input, const input_t& last_input) const noexcept {
    bool pc_pressed = (cur_input.keys[pc] && !last_input.keys[pc]);
    bool mouse_pressed = (cur_input.mouse[mouse] && !last_input.mouse[mouse]);
    bool gamepad_pressed = (cur_input.gamepad[gamepad] && !last_input.gamepad[gamepad]);

    // Ignore bindings that never had a value assigned
    // Sorry for the confusing variable names here...
    pc_pressed *= pc != KEY_UNKNOWN;
    mouse_pressed *= mouse != MOUSE_UNKNOWN;
    gamepad_pressed *= gamepad != GAMEPAD_UNKNOWN;

    return pc_pressed || mouse_pressed || gamepad_pressed;
}

bool bind::falling_edge(const input_t& cur_input, const input_t& last_input) const noexcept {
    // Re-use the rising edge code, but swap around its inputs.
    return this->rising_edge(last_input, cur_input);
}

bool entry::rising_edge(const input_t& cur_input, const input_t& last_input) const noexcept {
    return main.rising_edge(cur_input, last_input) || alt.rising_edge(cur_input, last_input);
}

bool entry::falling_edge(const input_t& cur_input, const input_t& last_input) const noexcept {
    // Re-use the rising edge code, but swap around its inputs.
    return this->rising_edge(last_input, cur_input);
}

} // namespace action
