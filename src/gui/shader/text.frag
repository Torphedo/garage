#version 330 core
out vec4 fragment_rgba;

in vec2 texcoord;

uniform sampler2D font_atlas;

void main() {
    fragment_rgba = texture(font_atlas, texcoord);
}
