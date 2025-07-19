#version 430 core

layout (location = 0) in vec3 aPos;  // Static unit circle geometry

layout(std430, binding = 1) buffer Positions { vec3 positions[]; };

layout(std430, binding = 3) buffer Velocities { vec3 velocities[]; };

uniform float scale;

out vec2 texCoord;
out vec3 velocity;

void main()
{
    vec3 instancePos = positions[gl_InstanceID];
    vec3 instanceVel = velocities[gl_InstanceID];
    
    vec3 worldPos = instancePos + aPos * scale;

    gl_Position = vec4(worldPos, 1.0);

    texCoord = aPos.xy * 0.5 + 0.5;
    velocity = instanceVel;
}
