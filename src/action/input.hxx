#pragma once
/// @brief Input state & polling callbacks

#include <GLFW/glfw3.h>
#include <cglm/struct.h>
#include "keys.hxx"

// TLDR: We have to keep state ourselves because relying on the callback creates
// a delay between pressing a button and it being considered "held", which feels
// terrible to use.

// Sorry for the counter-intuitive namespace name, "input" was taken by one of
// my other libraries where I made the mistake of defining an un-namespaced
// global variable. It'll be fixed Eventually(tm), but a lot of things depend
// on the global input struct. - torph
namespace action {

// Complete input state for the keyboard, mouse, and controller(s)
struct input_t {
    // Keyboard keys
    bool keys[KEY_LAST];

    // Modifier keys
    bool shift;
    bool control;
    bool alt;
    bool super; // AKA Windows key
    bool caps_lock;
    bool num_lock;

    // Mouse buttons
    bool mouse[MOUSE_LAST];

    vec2s cursor;
    vec2s scroll;

    // Gamepad left stick
    vec2s LS;
    // Gamepad right stick
    vec2s RS;
    // Gamepad left trigger
    float LT;
    // Gamepad right trigger
    float RT;
    // Gamepad buttons
    bool gamepad[GAMEPAD_BUTTON_LAST];

};

/// GLFW callback triggered on keyboard input
/// For glfwSetKeyCallback()
void update_key(GLFWwindow* window, int key, int scancode, int action, int mods);

/// GLFW callback triggered on mouse input
/// For glfwSetCursorPosCallback()
void update_cursor(GLFWwindow* window, double xpos, double ypos);

/// GLFW callback triggered on scroll wheel input
/// For glfwSetScrollCallback()
void update_scroll(GLFWwindow* window, double x, double y);

/// GLFW callback triggered on mouse button press
/// For glfwSetMouseButtonCallback()
void update_mouse_button(GLFWwindow* window, int button, int action, int mods);

/// Update modifier keys (shift, control, alt). Call this every frame.
void update_mods(GLFWwindow* window);

/// Collect gamepad state from up to 16 connected controllers
void update_gamepad();

/// Call this at the end of the frame to keep track of the previous frame's input.
void input_end_frame();

/// Globally accessible current input data
extern input_t input;

/// Globally accessible input data from last frame
extern input_t prev_input;

// Check if a key went from released to pressed this frame
bool rising_edge(keyboard_key key, const input_t& cur_input = input, const input_t& last_input = prev_input);

// Check if a button went from released to pressed this frame
bool rising_edge(gamepad_button button, const input_t& cur_input = input, const input_t& last_input = prev_input);

bool rising_edge(mouse_button button, const input_t& cur_input = input, const input_t& last_input = prev_input);

// Check if a key went from pressed to released this frame
template <typename T>
bool falling_edge(T key, const input_t& cur_input = input, const input_t& last_input = prev_input) {
    // This re-uses all rising edge overloads
    return rising_edge(key, last_input, cur_input);
}

/// Deadzone for controllers
static constexpr float deadzone = 0.25f;

} // namespace action
