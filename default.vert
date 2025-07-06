#version 330 core
layout (location = 0) in vec3 aPos;       // Circle vertex
layout (location = 1) in vec3 iOffset;    // Particle position

uniform float radius;

void main() {
    vec3 scaled = aPos * radius;
    gl_Position = vec4(scaled + iOffset, 1.0);
}
