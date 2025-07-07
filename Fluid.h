#ifndef FLUID_CLASS_H
#define FLUID_CLASS_H


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <vector>
#include <algorithm>

const float PI = 3.14159265359f;

struct Particle {
	glm::vec3 position; // x, y, z
	glm::vec3 velocity; // vx, vy, vz
	float radius;

	Particle() : position(0.0f), velocity(0.0f), radius(0.1f) {} // Default constructor
};

class Fluid {
	private :
		std::vector<Particle> _particles; // List of particles in the fluid
		std::vector<float> _particleProperties; // Density of each particle
		std::vector<float> _densities;

		float _particleCount; // Number of particles in the fluid
		float _mass; // Default mass, can be adjusted
		float _gravityAcceleration; // Gravity acceleration in m/s^2
		float _collisionDamping; // Damping factor to simulate collision response
		float _smoothingRadius; // Smoothing radius for density calculations
		float _targetDensity;
		float _pressureMultiplier;

	public:
		Fluid(const unsigned int particleCount, const float mass, const float gravity, const float collisionDamping, float spacing, float pressureMultiplier, float targetDensity, float smoothingRadius);
		void Update(float dt);
		void GetParticlePositions(std::vector<glm::vec3>& outPositions);
		const glm::vec3& GetPosition(int i) const;
		const glm::vec3& GetVelocity(int i) const;
		void HandleBoundaryCollisions(float boundryX, float boundaryY, float boundaryZ);
		static float SmoothingKernel(float radius, float distance);
		static float SmoothingKernelDerivative(float radius, float distance);
		float CalculateDensity(const glm::vec3& samplePosition) const;
		float DensityToPressure(float density);
		glm::vec3 CalculatePressureForce(int particleIndex);
		float CalculateSharedPressure(float densityA, float densityB);
		static glm::vec3 GetRandomDirection3D();
};

#endif
