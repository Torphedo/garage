#include "input.hxx"
#include <cstring>

#include <common/int.h>
#include <common/logging.h>
#include "keys.hxx"

namespace action {
input_t input = {0};
input_t prev_input = {0};

void update_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // RELEASE is 0, and both PRESS and REPEAT are > 0. So we can pass the
    // action code directly as a bool, and it will toggle the state correctly.
    const bool state = action != GLFW_RELEASE;
    assert(key <= ARRAY_SIZE(input.keys));
    input.keys[key] = state;

    // Toggle the modifier keys if they're being held
    input.shift     = (!input.shift & (mods & GLFW_MOD_SHIFT));
    input.control   = (!input.control & (mods & GLFW_MOD_CONTROL));
    input.alt       = (!input.alt & (mods & GLFW_MOD_ALT));
    input.super     = (!input.super & (mods & GLFW_MOD_SUPER));
    input.caps_lock = (!input.caps_lock & (mods & GLFW_MOD_CAPS_LOCK));
    input.num_lock  = (!input.num_lock & (mods & GLFW_MOD_NUM_LOCK));
}

void update_scroll(GLFWwindow* window, double x, double y) {
    input.scroll.x += x;
    input.scroll.y += y;
}

void update_cursor(GLFWwindow* window, double x, double y) {
    input.cursor.x = x;
    input.cursor.y = y;
}

void update_mouse_button(GLFWwindow* window, int button, int action, int mods) {
    assert(button <= ARRAY_SIZE(input.mouse));
    input.mouse[button] = action;
}

void update_mods(GLFWwindow* window) {
    input.keys[KEY_LEFT_SHIFT] = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
    input.keys[KEY_RIGHT_SHIFT] = glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT);
    input.shift = input.keys[KEY_LEFT_SHIFT] | input.keys[KEY_RIGHT_SHIFT];

    input.keys[KEY_LEFT_CONTROL] = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL);
    input.keys[KEY_RIGHT_CONTROL] = glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL);
    input.control = input.keys[KEY_LEFT_CONTROL] | input.keys[KEY_RIGHT_CONTROL];

    input.keys[KEY_LEFT_ALT] = glfwGetKey(window, GLFW_KEY_LEFT_ALT);
    input.keys[KEY_RIGHT_ALT] = glfwGetKey(window, GLFW_KEY_RIGHT_ALT);
    input.alt = input.keys[KEY_LEFT_ALT] | input.keys[KEY_RIGHT_ALT];
}

void update_gamepad() {
    // Wipe previous gamepad state
    input.LS = {0};
    input.RS = {0};
    input.LT = 0;
    input.RT = 0;
    memset(&input.gamepad, 0, sizeof(input.gamepad));

    for (u8 i = 0; i < GLFW_JOYSTICK_LAST; i++) {
        if (!glfwJoystickIsGamepad(i)) {
            // If it doesn't have gamepad mappings, it's not usable for us
            continue;
        }

        GLFWgamepadstate gamepad = {0};
        glfwGetGamepadState(i, &gamepad); // Get input

        // Handle deadzone
        for (u8 i = GAMEPAD_AXIS_FIRST; i < GAMEPAD_AXIS_LAST; i++) {
            if (gamepad.axes[i] < deadzone) {
                gamepad.axes[i] = 0;
            }
        }

        // Sum up stick inputs from all available controllers
        input.LS.x += gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
        input.LS.y += gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        input.RS.x += gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
        input.RS.y += gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
        input.LT += gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
        input.RT += gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];

        // Set a button "on" if any of the controllers have it pressed
        for (u8 i = GAMEPAD_BUTTON_FIRST; i < GAMEPAD_BUTTON_LAST; i++) {
            input.gamepad[i] |= gamepad.buttons[i];
        }
    }

    // If for some reason someone is moving sticks on multiple controllers at
    // once, keep the results within range.
    input.LS.x = CLAMP(-1.0f, input.LS.x, 1.0f);
    input.LS.y = CLAMP(-1.0f, input.LS.y, 1.0f);
    input.RS.x = CLAMP(-1.0f, input.RS.x, 1.0f);
    input.RS.y = CLAMP(-1.0f, input.RS.y, 1.0f);
    input.LT = CLAMP(-1.0f, input.LT, 1.0f);
    input.RT = CLAMP(-1.0f, input.RT, 1.0f);
}

void input_end_frame() {
    prev_input = input;
}

bool rising_edge(keyboard_key key, const input_t& cur_input, const input_t& last_input) {
    return cur_input.keys[key] && !last_input.keys[key];
}

bool rising_edge(gamepad_button button, const input_t& cur_input, const input_t& last_input) {
    return cur_input.gamepad[button] && !last_input.gamepad[button];
}

bool rising_edge(mouse_button button, const input_t& cur_input, const input_t& last_input) {
    return cur_input.mouse[button] && !last_input.mouse[button];
}

} // namespace action
