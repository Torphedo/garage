#ifndef CAMERA_H
#define CAMERA_H
#include "gui_common.h"
#include <cglm/cglm.h>

void camera_update(gui_state* gui, mat4 *view);

// Vector of the direction the camera is looking
vec3s camera_facing();

void camera_set_target(vec3s pos);

// Get combined projection & view matrix for the current camera position
void camera_proj_view(gui_state* gui, mat4* out);

#endif // #ifndef CAMERA_H

