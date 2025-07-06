#ifndef FLUID_CLASS_H
#define FLUID_CLASS_H

#include <glm/glm.hpp>
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
		std::vector<Particle> particles; // List of particles in the fluid
		std::vector<float> particleProperties; // Density of each particle
		std::vector<float> densities;

		float particleCount = 0; // Number of particles in the fluid
		float mass = 1.0f; // Default mass, can be adjusted
		float gravity = 0.0f; // Gravity acceleration in m/s^2
		float collisionDamping = 1.0f; // Damping factor to simulate collision response
		float smoothingRadius = 0.1f; // Smoothing radius for density calculations

	public:
		Fluid(const unsigned int particleCount, const float gravity, const float collisionDamping, float spacing); // Removed qualified name
		void update(float dt);
		void getParticlePositions(std::vector<glm::vec3>& outPositions);
		const glm::vec3& getPosition(int i) const;
		const glm::vec3& getVelocity(int i) const;
		void applyForce(const glm::vec3& force);
		void resolveCollision(float boundryX, float boundaryY, float boundaryZ);
		static float smoothingKernel(float radius, float distance);
		static float smoothingKernelDerivative(float radius, float distance);
		float CalculateDensity(const glm::vec3& samplePosition) const;
		float CalculateProperty(const glm::vec3& samplePosition) const;
		void updateDensities();
		glm::vec3 calculatePropertyGradient(const glm::vec3& samplePosition);
};

#endif
