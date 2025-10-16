#pragma once
#include <cglm/struct.h>
#include <common/int.h>

// Up axis for our camera
static vec3s camera_up = {0.0f, 1.0f, 0.0f};

typedef enum : u8 {
    CAMERA_ORBIT, // 3rd-person dual-stick style
    CAMERA_POV, // POV Minecraft-style
    CAMERA_FLY, // Flying (freecam style, like Source Engine spectator)
    CAMERA_MODE_ENUM_MAX,
}camera_mode;

struct camera {
    vec3s target = {}; // Position the camera looks towards
    vec3s pos = {}; // Position of the viewer
    vec2s orbit_angles = {};
    float radius = 30.0f;
    float move_speed = 15.0f;
    float mouse_sens = 0.015f;

    bool invert_mouse_x = true;
    bool invert_mouse_y = false;
    camera_mode mode = CAMERA_ORBIT;

    /// @brief Updates the camera mode.
    ///
    /// Use this instead of accessing the field directly, otherwise it may break.
    /// @param cam The camera to modify
    /// @param new_mode The new mode to use
    void set_mode(camera_mode new_mode) noexcept;

    /// @brief Update the camera state (should be called each frame)
    /// @param The camera to modify
    /// @param delta_time Time elapsed since the last call
    void update(double delta_time) noexcept;

    /// @brief Gets the unit direction vector the camera is looking
    vec3s facing() const noexcept;

    // Get just the camera transform
    void view_matrix(mat4 view) const noexcept;

    // Get combined projection & view matrix for the current camera position
    void proj_view(mat4 out) const noexcept;

private:
    /// @brief Get camera position relative to the orbit center point
    vec3s orbit_pos_by_angles() const noexcept;

    /// @brief Screenspace cursor movement since last frame.
    ///
    /// This alos applies mouse inversion if needed, and the gamepad's right stick.
    vec2s get_cursor_delta(vec2s cursor_pos) const noexcept;
};
