#version 430 core

layout (location = 0) in vec3 aPos;  // Static unit circle geometry

layout(std430, binding = 1) buffer Positions { vec4 positions[]; };

layout(std430, binding = 3) buffer Velocities { vec4 velocities[]; };

uniform float scale; // Added for future adaptive sampling implementation
uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

out vec2 texCoord;
out vec3 velocity;

void main()
{
    vec3 instancePos = positions[gl_InstanceID].xyz;
    vec3 instanceVel = velocities[gl_InstanceID].xyz;
    
    vec3 worldPos = instancePos + aPos * scale;

    gl_Position = projection * view * model * vec4(worldPos, 1.0);

    texCoord = aPos.xy * 0.5 + 0.5;
    velocity = instanceVel;
}
