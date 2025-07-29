#ifndef FLUID_CLASS_H  

#define FLUID_CLASS_H  

#define GLM_ENABLE_EXPERIMENTAL 

#include "ComputeShader.h"
#include "SSBO.hpp"

#include <glm/glm.hpp>  
#include <glm/gtx/string_cast.hpp>  
#include <vector>   
#include <limits>  
#include <numeric>

const float PI = 3.14159265359f;
const float EPSILON = std::numeric_limits<float>::epsilon();
const unsigned int MAX_INT = std::numeric_limits<unsigned int>::max();


struct SimulationParameters {
	float dt;
	float gravityAcceleration;
	float mass;
	float collisionDamping;
	float smoothingRadius;
	float targetDensity;
	float pressureMultiplier;
	float viscosityStrength;
	float nearDensityMultiplier;
	uint32_t isInteracting;
	uint32_t isPaused;
	float inputPositionX;
	float inputPositionY;
	float inputPositionZ;
	float interactionRadius;
	float interactionStrength;

	uint32_t particleCount;
	uint32_t hashSize;
	float spacing;
	float particleRadius;
	float boundaryX;
	float boundaryY;
	float boundaryZ;

	// Padding to ensure the struct is 16 bytes aligned
	float padding;
};

struct Entry {
	int index;
	unsigned int key;
};

// Alligned to 16 bytes because of glm::vec4
struct ParticleVectors {
	glm::vec4 position;
	glm::vec4 predictedPosition;
	glm::vec4 velocity;
};

// Alligned to 4 bytes because of float
struct ParticleValues {
	float mass;
	float density;
	float nearDensity;
	uint32_t mergeFlag;
	uint32_t tag; // 0 = KEEP, 1 = SPLIT, 2 = MERGE

	// Padding to ensure the struct is 16 bytes aligned
	float padding1;     
	float padding2;    
	float padding3;  
};

class Fluid {  
	private : 
		SSBO <ParticleVectors> _particleVectors; // bind to 0
		SSBO <ParticleValues> _particleValues; // bind to 1

		//Ping-pong buffers for simulation steps
		SSBO <ParticleVectors> _newParticleVectors; // bind to 2
		SSBO <ParticleValues> _newParticleValues; // bind to 3

		SSBO <Entry> _spatialLookup; // bind to 4
		SSBO <unsigned int> _startIndices; // bind to 5
		SSBO <SimulationParameters> _simParams; // bind to 6

		GLuint _newParticleCounterBuffer; // bind to 7

		ComputeShader _predictedPosShader;
		ComputeShader _updateSpatialLookup;
		ComputeShader _densityStep;
		ComputeShader _forceStep;
		ComputeShader _fluidStep;
		ComputeShader _bitonicSortShader;
		ComputeShader _buildStartIndices;
		ComputeShader _tagParticles;
		ComputeShader _resampleParticles;

		SimulationParameters _params;
		
	public:  
		Fluid(unsigned int particleCount, float particleRadius, const float mass, const float gravity, const float collisionDamping, const float spacing, const float pressureMultiplier, const float targetDensity, const float smoothingRadius, const unsigned int hashSize, const float interactionRadius, const float interactionStrength, float viscosityStrength, float nearDensityMultiplier, float boundaryX, float boundaryY, float boundaryZ);

		void Update(float dt);

		void SortSpatialLookup();

		void BindRenderBuffers();

		// Get/set methods for mouse/keyboard controls
		void SetIsInteracting(bool state);
		void SetInteractionRadius(float radius);
		void SetInteractionPosition(glm::vec3 pos);
		void SetInteractionStrength(float strength);
		float GetPressureMultiplier();
		void SetPressureMultiplier(float pressureMultiplier);
		float GetTargetDensity();
		void SetTargetDensity(float targetDensity);
		float GetGravity();
		void SetGravity(float g);
		void SetPaused(bool isPaused);
		float GetViscosityStrength();
		void SetViscosityStrength(float strength);
		float GetNearDensityMultiplier();
		void SetNearDensityMultiplier(float nearDensityMultiplier);
};  

#endif // FLUID_CLASS_H
