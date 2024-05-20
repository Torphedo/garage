#version 330 core
out vec4 fragment_rgba;

in vec2 tex_coord;

// TODO: Make a default texture to upload & use here (see gl_box)
// uniform sampler2D perlin;

void main() {
    fragment_rgba = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    // fragment_rgba = texture(perlin, tex_coord);
}

