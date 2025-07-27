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
};

struct Entry {
	int index;
	unsigned int key;
};

class Fluid {  
	private : 
		SSBO <glm::vec4> _positions;
		SSBO <glm::vec4> _predictedPositions;
		SSBO <glm::vec4> _velocities;
		SSBO <float> _masses;
		SSBO <float> _densities;  
		SSBO <float> _nearDensities;
		SSBO <Entry> _spatialLookup;
		SSBO <unsigned int> _startIndices;
		SSBO <SimulationParameters> _simParams;

		//Ping-pong buffers for simulation steps
		SSBO <glm::vec4> _newPositions;
		SSBO <glm::vec4> _newPredictedPositions;
		SSBO <glm::vec4> _newVelocities;
		SSBO <float> _newMasses;
		SSBO <float> _newDensities;
		SSBO <float> _newNearDensities;
		SSBO <unsigned int> _tags;
		SSBO<uint32_t> _mergedFlags;

		ComputeShader _predictedPosShader;
		ComputeShader _updateSpatialLookup;
		ComputeShader _densityStep;
		ComputeShader _forceStep;
		ComputeShader _fluidStep;
		ComputeShader _bitonicSortShader;
		ComputeShader _buildStartIndices;
		ComputeShader _tagParticles;
		ComputeShader _resampleParticles;

		GLuint _newParticleCounterBuffer;

		SimulationParameters _params;

	public:  
		Fluid(unsigned int particleCount, float particleRadius, const float mass, const float gravity, const float collisionDamping, const float spacing, const float pressureMultiplier, const float targetDensity, const float smoothingRadius, const unsigned int hashSize, const float interactionRadius, const float interactionStrength, float viscosityStrength, float nearDensityMultiplier, float boundaryX, float boundaryY, float boundaryZ);

		void Update(float dt);

		void SortSpatialLookup();

		void BindRenderBuffers();

		// Setter/getter methods for keyboard controls
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
