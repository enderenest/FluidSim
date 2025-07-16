#include "Fluid.h"
#include <iostream>


Fluid::Fluid(const unsigned int particleCount, const float particleRadius, const float mass, const float gravityAcceleration, const float collisionDamping, const float spacing, const float pressureMultiplier, const float targetDensity, const float smoothingRadius, const unsigned int hashSize, const float interactionRadius, const float interactionStrength, float viscosityStrength, float nearDensityMultiplier, float boundaryX, float boundaryY, float boundaryZ)
    : _positions(particleCount, GL_DYNAMIC_DRAW),
    _velocities(particleCount, GL_DYNAMIC_DRAW),
    _densities(particleCount, GL_DYNAMIC_DRAW),
    _nearDensities(particleCount, GL_DYNAMIC_DRAW),
    _predictedPositions(particleCount, GL_DYNAMIC_DRAW),
    _startIndices(hashSize, GL_DYNAMIC_DRAW),
    _simParams(1, GL_DYNAMIC_DRAW),
    _updateShader("compute_shader.comp")
{
	//Initialize simulation parameters
    _params.dt = 0.016f;
    _params.gravityAcceleration = gravityAcceleration;
    _params.mass = mass;
    _params.collisionDamping = collisionDamping;
    _params.smoothingRadius = smoothingRadius;
    _params.targetDensity = targetDensity;
    _params.pressureMultiplier = pressureMultiplier;
    _params.viscosityStrength = viscosityStrength;
    _params.nearDensityMultiplier = nearDensityMultiplier;
    _params.isInteracting = 0;
    _params.isPaused = 0;
    _params.inputPosition = glm::vec3(0.0f);
    _params.interactionRadius = interactionRadius;
    _params.interactionStrength = interactionStrength;

	_params.particleCount = particleCount;
	_params.hashSize = hashSize;
	_params.spacing = spacing;
	_params.particleRadius = particleRadius;
	_params.boundaryX = boundaryX;
	_params.boundaryY = boundaryY;
	_params.boundaryZ = boundaryZ;

    _simParams.upload(std::vector<SimulationParameters>{_params});

    // Initialize positions in a grid
    std::vector<glm::vec3> initialPositions(particleCount, glm::vec3(0.0f));
    int particlesPerRow = static_cast<int>(std::sqrt(particleCount));
    int particlesPerCol = (particleCount - 1) / particlesPerRow + 1;

    for (unsigned int i = 0; i < particleCount; ++i) {
        int row = i / particlesPerRow;
        int col = i % particlesPerRow;
        float x = (col - particlesPerRow / 2.0f + 0.5f) * spacing;
        float y = (row - particlesPerCol / 2.0f + 0.5f) * spacing;
        initialPositions[i] = glm::vec3(x, y, 0.0f);
    }

    _positions.upload(initialPositions);
    _velocities.upload(std::vector<glm::vec3>(particleCount, glm::vec3(0.0f)));
    _densities.upload(std::vector<float>(particleCount, 0.0f));
    _nearDensities.upload(std::vector<float>(particleCount, 0.0f));
    _predictedPositions.upload(initialPositions);
    _startIndices.upload(std::vector<unsigned int>(hashSize, MAX_INT));

}

void Fluid::Update(float dt) {
    _updateShader.use();

    _simParams.upload(std::vector<SimulationParameters>{_params});

    _positions.bindTo(0);
    _predictedPositions.bindTo(1);
    _velocities.bindTo(2);
    _densities.bindTo(3);
    _nearDensities.bindTo(4);
    _startIndices.bindTo(6);
    _simParams.bindTo(7);

	const int groupSize = 32; // Nvidia recommends 32 for optimal performance. Please adjust based on your GPU model
    int numGroups = (_positions.count() + groupSize - 1) / groupSize;
    _updateShader.dispatch(numGroups);
}



void Fluid::SetIsInteracting(bool state) { _params.isInteracting = state; }
void Fluid::SetInteractionPosition(glm::vec3 pos) { _params.inputPosition = pos; }
void Fluid::SetInteractionStrength(float strength) { _params.interactionStrength = strength; }

float Fluid::GetPressureMultiplier() { return _params.pressureMultiplier; }
void Fluid::SetPressureMultiplier(float pressureMultiplier) { _params.pressureMultiplier = pressureMultiplier; }

float Fluid::GetTargetDensity() { return _params.targetDensity; }
void Fluid::SetTargetDensity(float targetDensity) { _params.targetDensity = targetDensity; }

void Fluid::SetGravity(float g) { _params.gravityAcceleration = g; }
float Fluid::GetGravity() {return _params.gravityAcceleration;}

void Fluid::SetPaused(bool isPaused) { _params.isPaused = isPaused; }

float Fluid::GetViscosityStrength() { return _params.viscosityStrength; }
void Fluid::SetViscosityStrength(float viscosityStrength) { _params.viscosityStrength = viscosityStrength; }

float Fluid::GetNearDensityMultiplier() { return _params.nearDensityMultiplier; }
void Fluid::SetNearDensityMultiplier(float nearDensityMultiplier) { _params.nearDensityMultiplier = nearDensityMultiplier; }







