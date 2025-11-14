#ifndef FLUID_CLASS_H  

#define FLUID_CLASS_H  

#define GLM_ENABLE_EXPERIMENTAL 

#include "ComputeShader.h"
#include "Mesh.h"
#include "SSBO.hpp"

#include <glm/glm.hpp> 
#include <glm/gtc/random.hpp>
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
	float jitter;

	uint32_t particleCount;
	uint32_t hashSize;
	float spacing;
	float particleRadius;
	float scaleX;
	float scaleY;
	float scaleZ;

	float boundaryCenterX;
	float boundaryCenterY;
	float boundaryCenterZ;
	float padding0; 

	float boundaryHalfX;
	float boundaryHalfY;
	float boundaryHalfZ;
	float padding1;
};

struct Entry {
	int index;
	unsigned int key;
};

class Fluid {  
	private : 
		// ---- immutable configuration ----
		const int   _particleCount;
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
		const float _jitterFraction;
		const float _scaleX, _scaleY, _scaleZ;

		// ---- SSBO Buffers ----
		SSBO <glm::vec4> _positions;			// bind to 1
		SSBO <glm::vec4> _predictedPositions;   // bind to 2
		SSBO <glm::vec4> _velocities;			// bind to 3
		SSBO <float> _densities;				// bind to 4
		SSBO <float> _nearDensities;			// bind to 5
		SSBO <Entry> _spatialLookup;			// bind to 6
		SSBO <unsigned int> _startIndices;		// bind to 7
		SSBO <SimulationParameters> _simParams;	// bind to 8
		SSBO<unsigned int> _wallImpacts;				// bind to 9

		ComputeShader _predictedPosShader;
		ComputeShader _updateSpatialLookup;
		ComputeShader _densityStep;
		ComputeShader _forceStep;
		ComputeShader _fluidStep;
		ComputeShader _bitonicSortShader;
		ComputeShader _buildStartIndices;

		SimulationParameters _params;

	public:  
		Fluid(float deltaTime, unsigned int particleCount, float particleRadius, float mass, float gravity, float collisionDamping, float spacing, float pressureMultiplier, float targetDensity, const float smoothingRadius, unsigned int hashSize, float interactionRadius, float interactionStrength, float viscosityStrength, float nearDensityMultiplier, float scaleX, float scaleY, float scaleZ, float jitter);

		void Update(float dt);

		void SortSpatialLookup();

		void BindRenderBuffers();

		void InitParticlesInsideCube(const Mesh& cubeMesh);

		void SetBoundsFromMesh(const Mesh& cubeMesh);

		void ResetWallImpacts();

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
