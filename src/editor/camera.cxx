#include <action/action.hxx>

#include "camera.hxx"

using namespace action;

vec3s camera::orbit_pos_by_angles() const noexcept {
    // Get combined quaternion of rotation about Y & Z axes
    const versors xrot = glms_quatv(orbit_angles.x, (vec3s){0, 1, 0});
    const versors yrot = glms_quatv(orbit_angles.y, (vec3s){0, 0, 1});
    const versors total_rot = glms_quat_mul(xrot, yrot);

    // Apply rotation to unit vector scaled by radius
    return glms_quat_rotatev(total_rot, (vec3s){radius,0,0});
}

vec2s camera::get_cursor_delta(vec2s cursor_pos) const noexcept {
    static vec2s last_cursor = {0};

    // Nullify movement unless click is held
    if (!action::input.mouse[MOUSE_LEFT]) {
        last_cursor.x = action::input.cursor.x;
        last_cursor.y = action::input.cursor.y;
    }

    vec2s cursor_delta = {
        .x = (cursor_pos.x - last_cursor.x) * mouse_sens,
        .y = (cursor_pos.y - last_cursor.y) * mouse_sens
    };

    // Save state so we can find the delta next time we're called
    last_cursor = cursor_pos;

    if (fabsf(action::input.RS.x) > deadzone || fabsf(action::input.RS.y) > deadzone) {
        cursor_delta.x = action::input.RS.x * mouse_sens * 5;
        cursor_delta.y = action::input.RS.y * mouse_sens * 5;
    }

    // Invert sign as needed.
    if (invert_mouse_x) {
        cursor_delta.x = -cursor_delta.x;
    }
    if (invert_mouse_y) {
        cursor_delta.y = -cursor_delta.y;
    }

    return cursor_delta;
}

void camera::update(double delta_time) noexcept {
    static vec2s last_scroll = {0};

    const vec2s scroll = {input.scroll.x, input.scroll.y};
    const vec2s cursor = {input.cursor.x, input.cursor.y};
    const vec2s cursor_delta = get_cursor_delta(cursor);

    const vec2s scroll_delta = {
        .x = input.scroll.x - last_scroll.x,
        .y = input.scroll.y - last_scroll.y
    };

    // Save state so we can find the delta next time we're called
    last_scroll = scroll;

    const vec3s cam_dir = facing();
    const float multiplier = delta_time * move_speed;

    // This just sets each axis to zero if it's below the deadzone threshold
    const float LS_x = input.LS.x * (fabsf(input.LS.x) > deadzone);
    const float LS_y = input.LS.y * (fabsf(input.LS.y) > deadzone);

    const float forward  = multiplier * ((input.keys[KEY_W] - input.keys[KEY_S]) - LS_y);
    const float side     = multiplier * ((input.keys[KEY_A] - input.keys[KEY_D]) - LS_x);
    float vertical = multiplier * ((input.keys[KEY_SPACE] - input.shift) + (input.RT - input.LT));

    // Exclude vertical view component so it doesn't affect horizontal movement
    vec3s horizontal = glms_normalize((vec3s){cam_dir.x, 0, cam_dir.z});
    const vec3s cam_side = glms_vec3_rotate(horizontal, glm_rad(90), camera_up);
    // Make forward/back move along camera vector in fly mode
    if (mode == CAMERA_FLY) {
        horizontal = cam_dir;
        vertical = 0; // Ignore the normal vertical movement keys
    }

    vec3s pos_delta = glms_vec3_scale(horizontal, forward); // [Camera dir] * forward movement
    // Add [Camera dir rotated by 90 degrees] * side movement
    pos_delta = glms_vec3_add(pos_delta, glms_vec3_scale(cam_side, side));
    pos_delta.y += vertical;

    // Update angles & zoom from mouse input
    orbit_angles = glms_vec2_add(orbit_angles, cursor_delta);
    radius -= scroll_delta.y;
    radius = CLAMP(0.05f, radius, 256.0f); // Don't allow <= 0 or really high zoom

    // Update target pos using delta from user input
    target = glms_vec3_add(target, pos_delta);

    // Rendering breaks @ exactly 90 with Euler rotations, and we don't want to
    // be upside-down.
    orbit_angles.y = CLAMP(glm_rad(-89.999f), orbit_angles.y, glm_rad(89.999f));

    // Add target position to relative orbit position to get final position
    pos = glms_vec3_add(target, orbit_pos_by_angles());
}

vec3s camera::facing() const noexcept {
    // In fly mode, the target & camera are swapped
    const vec3s new_target = (mode == CAMERA_ORBIT) ? target : pos;
    const vec3s new_pos = (mode == CAMERA_ORBIT) ? pos : target;
    return glms_normalize(glms_vec3_sub(new_target, new_pos));
}

void camera::set_mode(camera_mode new_mode) noexcept {
    if (new_mode == mode) {
        return; // Nothing to do.
    }

    switch (new_mode) {
        case CAMERA_ORBIT:
            invert_mouse_x = true;
            invert_mouse_y = false;
            mouse_sens = 0.015f;
            break;
        default:
        case CAMERA_FLY:
            invert_mouse_x = true;
            invert_mouse_y = true;
            mouse_sens = 0.005f;
            break;
    }
    // If entering or leaving orbit mode, the target will be swapped with the
    // camera. We need to face the opposite direction to correct for the change
    bool needs_view_flip = (mode == CAMERA_ORBIT || new_mode == CAMERA_ORBIT);
    if (needs_view_flip) {
        orbit_angles.x = fmodf(orbit_angles.x + glm_rad(180), 360);
        orbit_angles.y = -orbit_angles.y;
    }
    
    // Set mode
    mode = new_mode;
}

void camera::view_matrix(mat4 view_out) const noexcept {
    if (mode == CAMERA_ORBIT) {
        glm_lookat((float*)&pos, (float*)&target, (float*)&camera_up, view_out);
    } else {
        // In fly mode, the target & camera are swapped
        glm_lookat((float*)&target, (float*)&pos, (float*)&camera_up, view_out);
    }
}

void camera::proj_view(mat4 out) const noexcept {
    // Projection matrix
    mat4 projection = {0};
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    const float aspect = (float)mode->width / (float)mode->height;
    glm_perspective_rh_no(glm_rad(45), aspect, 0.1f, 1000.0f, projection);

    // Camera matrix
    mat4 view = {0};
    view_matrix(view);
    glm_mat4_mul(projection, view, (vec4*)out);
}
