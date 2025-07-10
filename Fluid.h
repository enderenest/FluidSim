#ifndef FLUID_CLASS_H  

#define FLUID_CLASS_H  

#define GLM_ENABLE_EXPERIMENTAL  
#include <glm/glm.hpp>  
#include <glm/gtx/string_cast.hpp>  
#include <vector>  
#include <algorithm>  
#include <limits>  
#include <unordered_map>
#include <numeric>
#include <execution>
#include <omp.h>

const float PI = 3.14159265359f;
const float twoPI = 2.0f * PI;
const float EPSILON = std::numeric_limits<float>::epsilon();
const unsigned int MAX_INT = std::numeric_limits<unsigned int>::max();


class Entry {
public:
	int index;
	unsigned int key;

	Entry() : index(-1), key(0) {}
	Entry(int i, unsigned int k) : index(i), key(k) {}

	bool operator<(const Entry& other) const {
		return key < other.key;
	}
};

static std::pair<int, int> cellOffsets[9] = {
	{0, 0}, {1, 0}, {0, 1},
	{-1, 0}, {0, -1}, {1, 1},
	{-1, -1}, {1, -1}, {-1, 1}
};


class Fluid {  
	private :  
		std::vector<glm::vec3> _positions;
		std::vector<glm::vec3> _velocities; 
		std::vector<float> _densities;  
		std::vector<glm::vec3> _predictedPositions;
		std::vector<Entry> _spatialLookup;
		std::vector<unsigned int> _startIndices;


		unsigned int _particleCount; // Number of particles in the fluid
		unsigned int _hashSize; // Size of the spatial hash table
		float _particleRadius; // Radius of each particle
		float _mass; // Default mass, can be adjusted  
		float _gravityAcceleration; // Gravity acceleration in m/s^2  
		float _collisionDamping; // Damping factor to simulate collision response  
		float _smoothingRadius; // Smoothing radius for density calculations  
		float _targetDensity;  
		float _pressureMultiplier;
		float _viscosityStrength;
		float _nearDensity;
		float _radius2;
		float _radius4;
		float _radius8;
		float _formulaConstant;

		bool _isPaused = false;

		float _interactionRadius;
		float _interactionStrength;

	public:  
		Fluid(unsigned int particleCount, float particleRadius, const float mass, const float gravity, const float collisionDamping, const float spacing, const float pressureMultiplier, const float targetDensity, const float smoothingRadius, const unsigned int hashSize, const float interactionRadius, const float interactionStrength, float viscosityStrength, float nearDensity);  
		void Update(float dt);  
		void GetParticlePositions(std::vector<glm::vec3>& outPositions);
		void GetParticleVelocities(std::vector<glm::vec3>& outVelocities);
		float GetPressureMultiplier();
		void SetPressureMultiplier(float pressureMultiplier);
		float GetTargetDensity();
		void SetTargetDensity(float targetDensity);
		float GetGravity();
		void SetGravity(float g);
		void SetPaused(bool isPaused);
		float GetViscosityStrength();
		void SetViscosityStrength(float strength);

		void HandleBoundaryCollisions(float boundryX, float boundaryY, float boundaryZ);  
		float SmoothingKernel(float radius, float distance);  
		float SmoothingKernelDerivative(float radius, float distance);  
		float CalculateDensity(const int i);  
		float DensityToPressure(float density);  
		glm::vec3 CalculatePressureForce(int particleIndex);  
		float CalculateSharedPressure(float densityA, float densityB);  
		static glm::vec3 GetRandomDirection3D();
		glm::ivec3 PositionsToCellCoord(glm::vec3 point, float radius);
		unsigned int HashCell(int cellX, int cellY);
		unsigned int GetKeyFromHash(unsigned int hash);
		void UpdateSpatialLookup(float radius);
		void ApplyInteractionForce(glm::vec2 inputPos, float radius, float strength);
		float ViscosityKernel(float radius, float distance);
		float ViscosityKernelDerivative(float radius, float distance);
		glm::vec3 CalculateViscosityForce(int particleIndex);
};  

#endif // FLUID_CLASS_H
