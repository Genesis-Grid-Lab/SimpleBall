#version 330

in vec3 vertexPosition;
uniform mat4 mvp; // Matrix Light Projection * Light View * Model

void main() {
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
