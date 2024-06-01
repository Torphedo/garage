#version 110
// Vertex color passed from vertex shader
varying vec4 vert_color;

void main() {
    gl_FragColor = vert_color;
}

