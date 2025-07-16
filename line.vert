#version 430 core

layout (location = 0) in vec3 aPos;

layout(std430, binding = 0) buffer Positions {
    vec3 positions[];
};

void main()
{
    vec3 instancePos = positions[gl_InstanceID];
    gl_Position = vec4(instancePos, 1.0);
}