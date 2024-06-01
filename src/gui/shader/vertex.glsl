#version 110
// Vertex attributes
attribute vec3 a_pos;
attribute vec4 a_color;

// Export vert color to fragment shader
varying vec4 vert_color;

uniform mat4 pvm;
uniform vec4 paint;

void main() {
    gl_Position = pvm * vec4(a_pos, 1.0);
    vert_color = a_color * paint;
}

