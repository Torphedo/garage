#include <cglm/cglm.h>

#include <common/input.h>

#include "camera.h"
#include "gui_common.h"

// Restrict a number to a certain range
float clampf(float x, float min, float max) {
    if (x > max) {
        return max;
    }
    else if (x < min) {
        return min;
    }
    else {
        return x;
    }
}

// Lengthen/shorten a vector by an arbitrary amount.
vec2s vec2_addmag(vec2s v, float amount) {
    float magnitude = glms_vec2_norm(v);
    // Turn our amount into a scalar we can multiply the vector by
    float scalar = 1.0f - (amount / magnitude);
    
    // Scale the vector.
    v = glms_vec2_scale(v, scalar);
    return v;
}

// Get the camera position relative to an orbit center-point based on the
// rotation angles
vec3s orbit_pos_by_angles(camera* cam, vec2s angles, float orbit_radius) {
    // Get XYZ positions using trig on our angles
    cam->pos = (vec3s){
        .x = sinf(cam->orbit_angles.x),
        .z = cosf(cam->orbit_angles.x),
        .y = sinf(cam->orbit_angles.y)
    };

    // To keep the camera at a constant distance we need to move our horizontal
    // position vector towards (0, 0) by the distance between the radius and the
    // Z position of our Y angle.
    float z_diff = (1.0f - cosf(cam->orbit_angles.y));

    // Make sure we're SHORTENING the vector, not adding to it.
    z_diff = fabsf(z_diff); 
    
    // Shorten our horizontal vector by the calculated amount.
    vec2s horizontal_pos = {cam->pos.x, cam->pos.z};
    horizontal_pos = vec2_addmag(horizontal_pos, z_diff);
    
    cam->pos.x = horizontal_pos.x;
    cam->pos.z = horizontal_pos.y;
    
    // Scale our position to the orbit radius
    cam->pos = glms_vec3_scale_as(cam->pos, orbit_radius);

    return cam->pos;
}

vec2s get_cursor_delta(camera* cam, vec2s cursor_pos) {
    static vec2s last_cursor = {0};

    // Nullify movement unless click is held
    if (!input.click_left) {
        last_cursor = input.cursor;
    }

    vec2s cursor_delta = {
        .x = (cursor_pos.x - last_cursor.x) * cam->mouse_sens,
        .y = (cursor_pos.y - last_cursor.y) * cam->mouse_sens
    };

    // Save state so we can find the delta next time we're called
    last_cursor = cursor_pos;

    // Invert sign as needed.
    if (cam->invert_mouse_x) {
        cursor_delta.x = -cursor_delta.x;
    }
    if (cam->invert_mouse_y) {
        cursor_delta.y = -cursor_delta.y;
    }

    return cursor_delta;
}

// Don't use this. Kinda sucks.
void update_roll(float delta_time, float angle_diff) {
    static const vec3s axis_forward = {0.0f, 0.0f, 1.0f};
    float roll = angle_diff * 5.0f * delta_time;
    camera_up = glms_vec3_rotate(camera_up, roll, axis_forward);
}

void camera_update(camera* cam, double delta_time) {
    static vec2s last_scroll = {0};

    const vec2s cursor_delta = get_cursor_delta(cam, input.cursor);

    const vec2s scroll_delta = {
        .x = input.scroll.x - last_scroll.x,
        .y = input.scroll.y - last_scroll.y
    };

    vec3s cam_dir = glms_normalize(camera_facing(*cam));
    float multiplier = delta_time * cam->move_speed;
    float forward  = multiplier * (input.w - input.s);
    float side     = multiplier * (input.a - input.d);
    float vertical = multiplier * (input.space - input.shift);

    // Exclude vertical view component so it doesn't affect horizontal movement
    vec3s horizontal = glms_normalize((vec3s){cam_dir.x, 0, cam_dir.z});
    vec3s cam_side = glms_vec3_rotate(horizontal, glm_rad(90), camera_up);
    // Make forward/back move along camera vector in fly mode
    if (cam->mode == CAMERA_FLY) {
        horizontal = cam_dir;
        vertical = 0; // Ignore the normal vertical movement keys
    }

    vec3s pos_delta = glms_vec3_scale(horizontal, forward); // [Camera dir] * forward movement
    // Add [Camera dir rotated by 90 degrees] * side movement
    pos_delta = glms_vec3_add(pos_delta, glms_vec3_scale(cam_side, side));
    pos_delta.y += vertical;

    // Save state so we can find the delta next time we're called
    last_scroll = input.scroll;

    // Update angles & zoom from mouse input
    cam->orbit_angles = glms_vec2_add(cam->orbit_angles, cursor_delta);
    cam->radius -= scroll_delta.y;
    cam->radius = clampf(cam->radius, 0.05f, 256.0f); // Don't allow <= 0 or really high zoom

    // Update target pos using delta from user input
    cam->target = glms_vec3_add(cam->target, pos_delta);

    // Rendering breaks at exactly 90 and we don't want to be upside-down
    cam->orbit_angles.y = clampf(cam->orbit_angles.y, glm_rad(-89.999f), glm_rad(89.999f));
    
    // Add target position to relative orbit position
    cam->pos = glms_vec3_add(cam->target, orbit_pos_by_angles(cam, cam->orbit_angles, cam->radius));
}

vec3s camera_facing(camera cam) {
    if (cam.mode == CAMERA_ORBIT) {
        return glms_vec3_sub(cam.target, cam.pos);
    } else {
        // In fly mode, the target & camera are swapped
        return glms_vec3_sub(cam.pos, cam.target);
    }
}

void camera_set_mode(camera* cam, camera_mode mode) {
    if (mode == cam->mode) {
        return;
    }

    switch (mode) {
        case CAMERA_ORBIT:
            cam->invert_mouse_x = true; 
            cam->invert_mouse_y = false;
            cam->mouse_sens = 0.015f;
            break;
        default:
        case CAMERA_FLY:
            cam->invert_mouse_x = true; 
            cam->invert_mouse_y = true;
            cam->mouse_sens = 0.005f;
            break;
    }
    // If endering or leaving orbit mode, the target will be swapped with the
    // camera. We need to face the opposite direction to correct for the change
    bool needs_view_flip = (cam->mode == CAMERA_ORBIT || mode == CAMERA_ORBIT);
    if (needs_view_flip) {
        cam->orbit_angles.x = fmodf(cam->orbit_angles.x + glm_rad(180), 360);
        cam->orbit_angles.y = -cam->orbit_angles.y;
    }
    
    // Set mode
    cam->mode = mode;
}

void camera_set_target(camera* cam, vec3s pos) {
    cam->target = (vec3s){
        .x = pos.x,
        .y = pos.y,
        .z = pos.z,
    };
}

void camera_view_matrix(camera cam, mat4 view) {
    if (cam.mode == CAMERA_ORBIT) {
        glm_lookat((float*)&cam.pos, (float*)&cam.target, (float*)&camera_up, view);
    } else {
        // In fly mode, the target & camera are swapped
        glm_lookat((float*)&cam.target, (float*)&cam.pos, (float*)&camera_up, view);
    }
}

void camera_proj_view(camera cam, mat4 out) {
    // Pre-multiply the projection & view components of the PVM matrix

    // Projection matrix
    mat4 projection = {0};
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    float aspect = (float)mode->width / (float)mode->height;
    glm_perspective_rh_no(glm_rad(45), aspect, 0.1f, 1000.0f, projection);

    // Camera matrix
    mat4 view = {0};
    camera_view_matrix(cam, view);
    glm_mat4_mul(projection, view, (vec4*)out);
}

