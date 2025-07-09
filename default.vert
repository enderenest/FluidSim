#version 330 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 instancePos;

uniform float scale;
uniform float radius;

out vec2 uv;
out float speedT;

uniform sampler1D speedColorMap;
uniform float velocityMax;

// Replace with actual velocity buffer if needed
uniform vec3 velocities[1500]; // assuming max PARTICLE_COUNT

void main()
{
    uv = (vertex.xy / (2.0 * radius)) + 0.5;

    float3 centreWorld = instancePos;
    float3 worldVertPos = centreWorld + vertex * scale;
    gl_Position = vec4(worldVertPos, 1.0);

    float speed = length(velocities[gl_InstanceID]);
    speedT = clamp(speed / velocityMax, 0.0, 1.0);
}
