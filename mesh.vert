#version 430 core

layout (location = 0) in vec3 aPos;     // vertex position
layout (location = 1) in vec3 aNormal;  // vertex normal

out vec3 vWorldPos;
out vec3 vNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Position in world space
    vec4 worldPos = model * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    vNormal = mat3(model) * aNormal;
    gl_Position = projection * view * worldPos;
}
