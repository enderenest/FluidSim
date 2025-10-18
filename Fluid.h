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
const int MAX_INT = std::numeric_limits<int>::max();


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

	float high_density_factor;
	float low_density_factor;
	float max_mass_factor;
	float min_mass_factor;

	uint32_t cooldown_frames;

	uint32_t initialParticleCount;       // constant
	uint32_t currentParticleCount;	     // dynamic, can change due to merge/split
	uint32_t maxParticleCount;           // constant
	uint32_t lookupCapacity;             // constant, always a power of two >= maxParticleCount
	uint32_t paddedCurrentParticleCount; // dynamic, always a power of two >= currentParticleCount
	uint32_t hashSize;					 // constant
	uint32_t mergeSplitCoefficient;		 // constant, coefficient for max/min particle count
	float spacing;
	float particleRadius;
	float boundaryX;
	float boundaryY;
	float boundaryZ;

	float padding1;
	float padding2;
	float padding3;
};

struct Entry {
	unsigned int index;
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
	float particleRadius;
	uint32_t mergeFlag;
	uint32_t tag; // 0 = KEEP, 1 = SPLIT, 2 = MERGE
	uint32_t cooldown;
   
	float padding;
};

class Fluid {  
	private : 
		// ---- immutable configuration ----
		const int   _initialParticleCount;
		const int   _mergeSplitCoeff;
		const int   _cooldown_frames;
		const int   _hashSize;

		const float _particleRadius;
		const float _mass;
		const float _gravityAcceleration;
		const float _collisionDamping;
		const float _spacing;
		const float _pressureMultiplier;
		const float _targetDensity;
		const float _smoothingRadius;
		const float _interactionRadius;
		const float _interactionStrength;
		const float _viscosityStrength;
		const float _nearDensityMultiplier;
		const float _boundaryX, _boundaryY, _boundaryZ;
		const float _high_density_factor, _low_density_factor;
		const float _max_mass_factor, _min_mass_factor;

		// ---- immutable derived values ----
		const int   _maxParticleCount;      // initial * coeff
		const int   _lookupCapacity;        // nextPowerOfTwo(_maxParticleCount)

		// ---- SSBO Buffers ----
		SSBO <ParticleVectors> _particleVectors; // bind to 0
		SSBO <ParticleValues> _particleValues; // bind to 1

		// Ping-pong buffers for simulation steps
		SSBO <ParticleVectors> _newParticleVectors; // bind to 2
		SSBO <ParticleValues> _newParticleValues; // bind to 3

		SSBO <Entry> _spatialLookup; // bind to 4
		SSBO <int> _startIndices; // bind to 5
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
		ComputeShader _resetMergeFlags;

		SimulationParameters _params;
		
	public:  
		Fluid(int initialParticleCount, int mergeSplitCount, int cooldown_frames,  float particleRadius, const float mass,  float gravity,  float collisionDamping,  float spacing,  float pressureMultiplier,  float targetDensity,  float smoothingRadius,  int hashSize,  float interactionRadius,  float interactionStrength,  float viscosityStrength,  float nearDensityMultiplier,  float boundaryX,  float boundaryY,  float boundaryZ,  float high_density_factor,  float low_density_factor,  float max_mass_factor,  float min_mass_factor);

		void Update(float dt);

		void UpdateSpatialHashing(int groups);

		void resetParticleCounter();
		static GLuint nextPowerOfTwo(GLuint x);
		void SortSpatialLookup();

		void BindRenderBuffers();

		// Get/set methods for mouse/keyboard controls
		void SetIsInteracting(bool state);
		void SetInteractionRadius(float radius);
		void SetInteractionPosition(glm::vec3 pos);
		void SetInteractionStrength(float strength);
		float GetPressureMultiplier() const;
		void SetPressureMultiplier(float pressureMultiplier);
		float GetTargetDensity() const;
		void SetTargetDensity(float targetDensity);
		float GetGravity() const;
		void SetGravity(float g);
		void SetPaused(bool isPaused);
		float GetViscosityStrength() const;
		void SetViscosityStrength(float strength);
		float GetNearDensityMultiplier() const;
		void SetNearDensityMultiplier(float nearDensityMultiplier);
		int GetParticleCount() const;
};  

#endif // FLUID_CLASS_H
