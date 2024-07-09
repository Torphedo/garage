#version 330 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_texcoord;

out vec2 texcoord;

uniform vec4 texcoords[8];
uniform mat4 transforms[8];

void main() {
    gl_Position = transforms[gl_InstanceID] * vec4(a_pos, 1.0);

    texcoord = a_texcoord;

    // Apply texcoords per instance
    if (texcoord.x == 0) {
        texcoord.x = texcoords[gl_InstanceID].r;
    }
    else {
        texcoord.x = texcoords[gl_InstanceID].b;
    }

    if (texcoord.y == 0) {
        texcoord.y = texcoords[gl_InstanceID].g;
    }
    else {
        texcoord.y = texcoords[gl_InstanceID].a;
    }
}
