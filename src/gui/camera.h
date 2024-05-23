#pragma once
#include "gui_common.h"
#include <cglm/cglm.h>

void camera_update(gui_state* gui, mat4 (*view));
vec3s camera_facing();
