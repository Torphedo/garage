#version 330 core
out vec4 fragment_rgba;

in vec4 vert_color;

void main() {
    fragment_rgba = vert_color;
}

