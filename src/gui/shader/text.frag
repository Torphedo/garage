#version 330 core
out vec4 fragment_rgba;

in vec2 texcoord;

uniform sampler2D font_atlas;

void main() {
    vec4 color = texture(font_atlas, texcoord);
    fragment_rgba = color.rrrr;
}
