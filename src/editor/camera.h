#ifndef CAMERA_H
#define CAMERA_H
#include <cglm/cglm.h>

// Up axis for our camera
static vec3s camera_up = {0.0f, 1.0f, 0.0f};

typedef enum {
    CAMERA_ORBIT, // 3rd-person dual-stick style
    CAMERA_POV, // POV Minecraft-style
    CAMERA_FLY, // Flying (freecam style, like Source Engine spectator)
    CAMERA_MODE_ENUM_MAX,
}camera_mode;

typedef struct {
    vec3s target; // Position the camera looks towards
    vec3s pos; // Position of the viewer
    vec2s orbit_angles;
    float radius;
    float move_speed;
    float mouse_sens;

    bool invert_mouse_x;
    bool invert_mouse_y;
    camera_mode mode;
}camera;

static camera camera_default() {
    return (camera) {
        .radius = 30.0f,
        .move_speed = 15.0f,
        .mouse_sens = 0.015f,
        .invert_mouse_x = true,
    };
}

/// @brief Updates the camera mode.
///
/// Use this instead of accessing the field directly, otherwise it may break.
/// @param cam The camera to modify
/// @param new_mode The new mode to use
void camera_set_mode(camera* cam, camera_mode new_mode);

/// @brief Update the camera state (should be called each frame)
/// @param The camera to modify
/// @param delta_time Time elapsed since the last call
void camera_update(camera* cam, double delta_time);

/// @brief Gets the unit direction vector the camera is looking
vec3s camera_facing(camera cam);

// Get just the camera transform
void camera_view_matrix(camera cam, mat4 view);

// Get combined projection & view matrix for the current camera position
void camera_proj_view(camera cam, mat4 out);

#endif // #ifndef CAMERA_H

