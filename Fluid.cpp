#include "Fluid.h"
#include <iostream>


Fluid::Fluid(const unsigned int particleCount, const float mass, const float gravity, const float collisionDamping, float spacing, float pressureMultiplier, float targetDensity, float smoothingRadius)
{
    _gravityAcceleration = gravity;
    _mass = mass;
    _collisionDamping = collisionDamping;
	_particleCount = particleCount;
	_pressureMultiplier = pressureMultiplier;
	_targetDensity = targetDensity;
    _smoothingRadius = smoothingRadius;

    _particles.reserve(particleCount);
	_particleProperties.resize(particleCount, 0.0f);
	_densities.resize(particleCount, 0.0f);

    int particlesPerRow = static_cast<int>(std::sqrt(particleCount));
    int particlesPerCol = (particleCount - 1) / particlesPerRow + 1;

    float particleSize = 0.05f;
    float particleSpacing = 0.01f;

    for (unsigned int i = 0; i < particleCount; ++i) {
        int row = i / particlesPerRow;
        int col = i % particlesPerRow;

        float x = (col - particlesPerRow / 2.0f + 0.5f) * spacing;
        float y = (row - particlesPerCol / 2.0f + 0.5f) * spacing;

        Particle particle;
        particle.position = glm::vec3(x, y, 0.0f);
        particle.velocity = glm::vec3(0.0f);
        particle.radius = particleSize;
        _particles.push_back(particle);
    }
	

}


void Fluid::Update(float dt) {
//#pragma omp parallel for
    for (int i = 0; i < _particleCount; ++i) {
		_particles[i].velocity += glm::vec3(0.0f, -_gravityAcceleration * dt, 0.0f);
        _densities[i] = CalculateDensity(_particles[i].position);
    }

//#pragma omp parallel for
    for (int i = 0; i < _particleCount; ++i) {
        glm::vec3 pressureForce = CalculatePressureForce(i);
        float density = _densities[i];
        glm::vec3 pressureAcceleration = pressureForce / density; // Calculate acceleration from force
        _particles[i].velocity = pressureAcceleration * dt; // Update velocity based on acceleration
    }

//#pragma omp parallel for
    for (int i = 0; i < _particleCount; ++i) {
		_particles[i].position += _particles[i].velocity * dt;
    }
}


void Fluid::HandleBoundaryCollisions(float boundaryX, float boundryY, float boundaryZ) {
    for (auto& particle : _particles) {
        // Check for collision with walls and resolve
        if (particle.position.x < -boundaryX || particle.position.x > boundaryX) { // Assuming the walls are at x = -1.0 and x = 1.0
            particle.position.x = glm::clamp(particle.position.x, -boundaryX, boundaryX); // Clamp position within bounds
            particle.velocity.x *= -_collisionDamping; // Reverse velocity and apply damping
        }
        if (particle.position.y < -boundryY || particle.position.y > boundryY) { // Assuming the walls are at y = -1.0 and y = 1.0
            particle.position.y = glm::clamp(particle.position.y, -boundryY, boundryY); // Clamp position within bounds
            particle.velocity.y *= -_collisionDamping; // Reverse velocity and apply damping
        }
        if (particle.position.z < -boundaryZ || particle.position.z > boundaryZ) { // Assuming the walls are at z = -1.0 and z = 1.0
            particle.position.z = glm::clamp(particle.position.z, -boundaryZ, boundaryZ); // Clamp position within bounds
            particle.velocity.z *= -_collisionDamping; // Reverse velocity and apply damping
        }
    }
}


void Fluid::GetParticlePositions(std::vector<glm::vec3>& outPositions) {
    outPositions.clear();
    for (const auto& p : _particles)
        outPositions.push_back(p.position);
}


const glm::vec3& Fluid::GetPosition(int i) const { // Returns the position as a reference to glm::vec3
    return _particles[i].position;
} 


const glm::vec3& Fluid::GetVelocity(int i) const { // Returns the velocity as a reference to glm::vec3
    return _particles[i].velocity;
}


float Fluid::SmoothingKernel(float radius, float distance) {  
	if (distance > radius) return 0.0f;

	float volume = (PI * std::pow(radius, 4)) / 6.0f;
    return (radius - distance) * (radius - distance) / volume;
}


float Fluid::SmoothingKernelDerivative(float radius, float distance) {
    if (distance > radius) return 0.0f;

    float scale = 12.0f / (PI * std::pow(radius, 4));
    return (distance - radius) * scale;
}


float Fluid::CalculateDensity(const glm::vec3& samplePosition) const {
    float density = 0.0f;

    for (const auto& particle : _particles) {
        float distance = glm::length(particle.position - samplePosition);
		float influence = SmoothingKernel(_smoothingRadius, distance);
		density += _mass * influence; // Assuming mass is 1.0f for all particles
    }
    return density;
}


float Fluid::DensityToPressure(float density) {
	float densityError = density - _targetDensity;
	float pressure = _pressureMultiplier * densityError;
	return pressure;
}


glm::vec3 Fluid::CalculatePressureForce(int particleIndex) {
    glm::vec3 pressureForce(0.0f);

    for (int otherIndex = 0; otherIndex < _particleCount; otherIndex++) {
		if (otherIndex == particleIndex) continue;

		glm::vec3 offset = _particles[otherIndex].position - _particles[particleIndex].position;
        float distance = glm::length(offset);
        glm::vec3 direction = (distance == 0) ? GetRandomDirection3D() : offset / distance;
        float slope = SmoothingKernelDerivative(_smoothingRadius, distance);
        float density = _densities[otherIndex];
		float avgPressure = CalculateSharedPressure(_densities[particleIndex], density);
        pressureForce += avgPressure * direction * slope * _mass / density;
    }

    return pressureForce;
}

float Fluid::CalculateSharedPressure(float densityA, float densityB) {
    float pressureA = DensityToPressure(densityA);
    float pressureB = DensityToPressure(densityB);
    return (pressureA + pressureB) / 2.0f;
}

glm::vec3 Fluid::GetRandomDirection3D() {
    float u = static_cast<float>(rand()) / RAND_MAX;
    float v = static_cast<float>(rand()) / RAND_MAX;

    float theta = 2.0f * PI * u;
    float phi = acos(2.0f * v - 1.0f);

    float x = sin(phi) * cos(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(phi);

    return glm::vec3(x, y, z);
}