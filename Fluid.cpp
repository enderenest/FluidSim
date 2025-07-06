#include "Fluid.h"
#include <iostream>


Fluid::Fluid(const unsigned int particleCount, const float gravity, const float collisionDamping, float spacing)
{
    this->gravity = gravity;
    this->collisionDamping = collisionDamping;
    particles.reserve(particleCount);

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
        particles.push_back(particle);
    }
}


void Fluid::update(float dt) {
    for (auto& particle : particles) {
        particle.position += particle.velocity * dt; // Update position based on velocity
        particle.velocity.y -= gravity * dt; // Apply gravity to the y component of velocity
	}
}

void Fluid::updateDensities() {
    #pragma omp parallel for
    for (int i = 0; i < particleCount; ++i) {
        float density = CalculateDensity(particles[i].position);
        densities[i] = density;
    }
}


void Fluid::applyForce(const glm::vec3& force) {
	for (auto& particle : particles) {
        glm::vec3 acceleration = force / mass; // Calculate acceleration from force
        particle.velocity += acceleration; // Update velocity based on acceleration
	}
}


void Fluid::resolveCollision(float boundaryX, float boundryY, float boundaryZ) {
    for (auto& particle : particles) {
        // Check for collision with walls and resolve
        if (particle.position.x < -boundaryX || particle.position.x > boundaryX) { // Assuming the walls are at x = -1.0 and x = 1.0
            particle.position.x = glm::clamp(particle.position.x, -boundaryX, boundaryX); // Clamp position within bounds
            particle.velocity.x *= -collisionDamping; // Reverse velocity and apply damping
        }
        if (particle.position.y < -boundryY || particle.position.y > boundryY) { // Assuming the walls are at y = -1.0 and y = 1.0
            particle.position.y = glm::clamp(particle.position.y, -boundryY, boundryY); // Clamp position within bounds
            particle.velocity.y *= -collisionDamping; // Reverse velocity and apply damping
        }
        if (particle.position.z < -boundaryZ || particle.position.z > boundaryZ) { // Assuming the walls are at z = -1.0 and z = 1.0
            particle.position.z = glm::clamp(particle.position.z, -boundaryZ, boundaryZ); // Clamp position within bounds
            particle.velocity.z *= -collisionDamping; // Reverse velocity and apply damping
        }
    }
}


void Fluid::getParticlePositions(std::vector<glm::vec3>& outPositions) {
    outPositions.clear();
    for (const auto& p : particles)
        outPositions.push_back(p.position);
}


const glm::vec3& Fluid::getPosition(int i) const { // Returns the position as a reference to glm::vec3
    return particles[i].position;
} 


const glm::vec3& Fluid::getVelocity(int i) const { // Returns the velocity as a reference to glm::vec3
    return particles[i].velocity;
}


float Fluid::smoothingKernel(float radius, float distance) {  
    float volume = PI * std::pow(radius, 8) / 4.0f;
    if (distance < radius) {  
        float value = std::max(0.0f, radius * radius - distance * distance);  
        return pow(value, 3) / volume;  
    }  
    return 0.0f; // Outside the smoothing kernel  
}


float Fluid::smoothingKernelDerivative(float distance, float radius) {
    if (distance > radius) return 0.0f;
	float value = radius * radius - distance * distance;
    float scale = -24.0f * (PI * std::pow(radius, 8));
    return scale * distance * std::pow(value, 2);
}


float Fluid::CalculateDensity(const glm::vec3& samplePosition) const {
    float density = 0.0f;

    for (const auto& particle : particles) {
        float distance = glm::length(particle.position - samplePosition);
		float influence = smoothingKernel(distance, smoothingRadius);
		density += mass * influence; // Assuming mass is 1.0f for all particles
    }
    return density;
}


float Fluid::CalculateProperty(const glm::vec3& samplePosition) const {
    float property = 0.0f;

    for (int i = 0; i < particleCount; i++) {
        float distance = glm::length(particles[i].position - samplePosition);
        float influence = smoothingKernel(distance, smoothingRadius);
        float density = CalculateDensity(particles[i].position);
        property += particleProperties[i] * influence * mass / density; // Assuming property is a function of influence
    }
    return property;
}

glm::vec3 Fluid::calculatePropertyGradient(const glm::vec3& samplePosition) {
    glm::vec3 propertyGradient(0.0f);

    for (const auto& particle : particles) {
        float distance = glm::length(particle.position - samplePosition);
        glm::vec3 direction = (samplePosition - particle.position) / distance;
        float slope = smoothingKernelDerivative(distance, smoothingRadius);
		float density = densities[&particle - &particles[0]];
        propertyGradient += slope * direction / distance; // Normalize the direction
      
    }
    return propertyGradient;
}






