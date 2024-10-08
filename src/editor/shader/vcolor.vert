#version 330 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec4 a_color;

out vec4 vert_color;

uniform mat4 pvm;
uniform vec4 paint;

void main() {
    gl_Position = pvm * vec4(a_pos, 1.0);
    vert_color = a_color * paint;
}
