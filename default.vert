#version 430 core

struct ParticleVectors {
	vec4 position;
	vec4 predictedPosition;
	vec4 velocity;
};

struct ParticleValues {
	float mass;
	float density;
	float nearDensity;
	float particleRadius;
	uint mergeFlag;
	uint tag; // 0 = KEEP, 1 = SPLIT, 2 = MERGE

	// Padding to ensure the struct is 16 bytes aligned
	float padding1;     
	float padding2;
};

layout (location = 0) in vec3 aPos;  // Static unit circle geometry

layout(std430, binding = 0) buffer VectorData { ParticleVectors vecData[]; };
layout(std430, binding = 1) buffer ValueData { ParticleValues valData[]; };

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

out vec2 texCoord;
out vec3 velocity;

void main()
{
    vec3 instancePos = vecData[gl_InstanceID].position.xyz;
    vec3 instanceVel = vecData[gl_InstanceID].velocity.xyz;
	float instanceScale = valData[gl_InstanceID].particleRadius;
    
    vec3 worldPos = instancePos + aPos * instanceScale;

    gl_Position = projection * view * model * vec4(worldPos, 1.0);

    texCoord = aPos.xy * 0.5 + 0.5;
    velocity = instanceVel;
}
