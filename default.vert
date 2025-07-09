#version 330 core

// Vertex input (quad vertex)
layout (location = 0) in vec3 aPos;
// Instance input: center position
layout (location = 1) in vec3 instancePos;
// Instance input: velocity
layout (location = 2) in vec3 instanceVel;

uniform float scale;

out vec2 texCoord;
out vec3 velocity;

void main()
{
    // Calculate scaled world position of vertex
    vec3 worldPos = instancePos + aPos * scale;

    // Set position to clip space
    gl_Position = vec4(worldPos, 1.0);

    // Pass UV and velocity
    texCoord = aPos.xy * 0.5 + 0.5;
    velocity = instanceVel;
}
