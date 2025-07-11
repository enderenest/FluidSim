#include "Fluid.h"
#include <iostream>


Fluid::Fluid(const unsigned int particleCount, const float particleRadius, const float mass, const float gravity, const float collisionDamping, const float spacing, const float pressureMultiplier, const float targetDensity, const float smoothingRadius, const unsigned int hashSize, const float interactionRadius, const float interactionStrength, float viscosityStrength, float nearDensityMultiplier)
{
    _gravityAcceleration = gravity;
    _mass = mass;
	_spacing = spacing;
    _collisionDamping = collisionDamping;
	_particleCount = particleCount;
	_pressureMultiplier = pressureMultiplier;
	_targetDensity = targetDensity;
    _smoothingRadius = smoothingRadius;
    _particleRadius = particleRadius;
    _hashSize = hashSize;
	_viscosityStrength = viscosityStrength;
	_nearDensityMultiplier = nearDensityMultiplier;
	_radius2 = _smoothingRadius * _smoothingRadius;
	_radius4 = _radius2 * _radius2;
    _radius8 = _radius4 * _radius4;
    _formulaConstant = (PI * _radius4);
	_interactionRadius = interactionRadius;
	_interactionStrength = interactionStrength;
    _spikyKernelPow2Factor = 15 / (2 * PI * std::pow(_smoothingRadius, 5));
    _derivativeSpikyPow2Factor = 15 / (PI * std::pow(_smoothingRadius, 5));
    _spikyKernelPow3Factor = 15 / (PI * std::pow(_smoothingRadius, 6));
    _derivativeSpikyPow3Factor = 45 / (PI * std::pow(_smoothingRadius, 6));

	_positions.resize(particleCount, glm::vec3(0.0f));
	_velocities.resize(particleCount, glm::vec3(0.0f));
	_densities.resize(particleCount, 0.0f);
    _nearDensities.resize(particleCount, 0.0f);
	_predictedPositions.resize(particleCount, glm::vec3(0.0f));
	_spatialLookup.resize(_particleCount, Entry());
	_startIndices.resize(_hashSize, MAX_INT);

    int particlesPerRow = static_cast<int>(std::sqrt(particleCount));
    int particlesPerCol = (particleCount - 1) / particlesPerRow + 1;

    for (unsigned int i = 0; i < particleCount; ++i) {
        int row = i / particlesPerRow;
        int col = i % particlesPerRow;

        float x = (col - particlesPerRow / 2.0f + 0.5f) * _spacing;
        float y = (row - particlesPerCol / 2.0f + 0.5f) * _spacing;

        _positions[i] = glm::vec3(x, y, 0.0f);
        _velocities[i] = glm::vec3(0.0f);
    }
}


void Fluid::Update(float dt) {
    if (_isPaused) {return; }

    #pragma omp parallel for
    for (int i = 0; i < _particleCount; ++i) {
		_velocities[i] += glm::vec3(0.0f, -_gravityAcceleration * dt, 0.0f);
        _predictedPositions[i] = _positions[i] + _velocities[i] * dt; // Predict next position
    }

	UpdateSpatialLookup(_smoothingRadius);

    #pragma omp parallel for
    for (int i = 0; i < _particleCount; ++i) {
		std::pair<float, float> densityPair = CalculateDensity(i);
        _densities[i] = densityPair.first;
		_nearDensities[i] = densityPair.second;
    }

    #pragma omp parallel for
    for (int i = 0; i < _particleCount; ++i) {
        glm::vec3 pressureForce = CalculatePressureForce(i);
        glm::vec3 viscosityForce = CalculateViscosityForce(i);
        float density = _densities[i];
        glm::vec3 pressureAcceleration = pressureForce / density; // Calculate acceleration from force
		glm::vec3 viscosityAcceleration = viscosityForce / density; // Calculate acceleration from viscosity force
        _velocities[i] += pressureAcceleration * dt; // Update velocity based on acceleration
		_velocities[i] += viscosityAcceleration * dt; // Update velocity based on viscosity force
    }

    #pragma omp parallel for
    for (int i = 0; i < _particleCount; ++i) {
		_positions[i] += _velocities[i] * dt;
    }
}


void Fluid::HandleBoundaryCollisions(float boundaryX, float boundaryY, float boundaryZ) {
    glm::vec2 halfBounds(boundaryX  - _particleRadius, boundaryY  - _particleRadius);

    for (int i = 0; i < static_cast<int>(_particleCount); i++) {
        if (std::abs(_positions[i].x) > halfBounds.x) {
            _positions[i].x = halfBounds.x * glm::sign(_positions[i].x);
            _velocities[i].x *= -_collisionDamping;
        }

        if (std::abs(_positions[i].y) > halfBounds.y) {
            _positions[i].y = halfBounds.y * glm::sign(_positions[i].y);
            _velocities[i].y *= -_collisionDamping;
        }

        _positions[i].z = 0.0f;
    }
}


float Fluid::SmoothingKernel(float radius, float distance) {  
	if (distance > radius) return 0.0f;

	float v = radius - distance;
    return v * v * _spikyKernelPow2Factor;
}


float Fluid::SmoothingKernelDerivative(float radius, float distance) {
    if (distance > radius) return 0.0f;

    float v = radius - distance;
    return -v * _spikyKernelPow2Factor;
}


std::pair<float, float> Fluid::CalculateDensity(int i)
{
    glm::vec3 cellCoord = PositionsToCellCoord(_predictedPositions[i], _smoothingRadius);
    float density = 0.0f;
	float nearDensity = 0.0f;

    int centerX = cellCoord.x;
    int centerY = cellCoord.y;
    float sqrRadius = _smoothingRadius * _smoothingRadius;

    for (int k = 0; k < 9; ++k) {
        int offsetX = cellOffsets[k].first;
        int offsetY = cellOffsets[k].second;

		unsigned int key = GetKeyFromHash(HashCell(centerX + offsetX, centerY + offsetY));
        int cellStartIndex = _startIndices[key];
		if (cellStartIndex == MAX_INT) continue;

        for (int j = cellStartIndex; j < _spatialLookup.size(); ++j) {
            if (_spatialLookup[j].key != key) break;

			int particleIndex = _spatialLookup[j].index;

            if (particleIndex == i) continue;

			glm::vec3 offset = _predictedPositions[particleIndex] - _predictedPositions[i];
			float sqrDistance = glm::dot(offset, offset);

            if (sqrDistance < sqrRadius) {
				float distance = std::sqrt(sqrDistance);
                density += SmoothingKernel(_smoothingRadius, distance) * _mass;
                nearDensity += NearSmoothingKernel(_smoothingRadius, distance) * _mass;
			}
        }
    }
    return std::make_pair(density, nearDensity);
}


float Fluid::DensityToPressure(float density) {
	float densityError = density - _targetDensity;
	float pressure = _pressureMultiplier * densityError;
    return pressure;
}


float Fluid::NearDensityToPressure(float nearDensity)
{
    return _nearDensityMultiplier * nearDensity;
}


glm::vec3 Fluid::CalculatePressureForce(int i) {
	glm::vec3 pressureForce(0.0f);
	glm::vec3 cellCoord = PositionsToCellCoord(_predictedPositions[i], _smoothingRadius);
    int centerX = cellCoord.x;
	int centerY = cellCoord.y;
	float sqrRadius = _smoothingRadius * _smoothingRadius;

    for (int k = 0; k < 9; ++k) {
        int offsetX = cellOffsets[k].first;
        int offsetY = cellOffsets[k].second;

		unsigned int key = GetKeyFromHash(HashCell(centerX + offsetX, centerY + offsetY));
		int cellStartIndex = _startIndices[key];
		if (cellStartIndex == MAX_INT) continue;

        for (int j = cellStartIndex; j < _spatialLookup.size(); ++j) {
            if (_spatialLookup[j].key != key) break;
			
			int particleIndex = _spatialLookup[j].index;
            if (particleIndex == i) continue;

			glm::vec3 offset = _predictedPositions[particleIndex] - _predictedPositions[i];
			float sqrDistance = glm::dot(offset, offset);

            if (sqrDistance < sqrRadius) {
				float distance = std::sqrt(sqrDistance);
                glm::vec3 direction = (distance == 0) ? GetRandomDirection3D() : offset / distance;
                float slope = SmoothingKernelDerivative(_smoothingRadius, distance);
				float nearSlope = NearSmoothingKernelDerivative(_smoothingRadius, distance);
                float density = _densities[particleIndex];
				float nearDensity = _nearDensities[particleIndex];
				float sharedPressure = CalculateSharedPressure(_densities[i], density); 
				float sharedNearPressure = CalculateNearSharedPressure(_nearDensities[i], nearDensity);
                pressureForce += sharedPressure * slope * direction * _mass / density;
                pressureForce += sharedNearPressure * nearSlope * direction * _mass / nearDensity;
			}
		}
    }

	return pressureForce;
}


float Fluid::CalculateSharedPressure(float densityA, float densityB) {
    float pressureA = DensityToPressure(densityA);
    float pressureB = DensityToPressure(densityB);
    return (pressureA + pressureB) / 2.0f;
}


float Fluid::CalculateNearSharedPressure(float densityA, float densityB) {
    float pressureA = NearDensityToPressure(densityA);
    float pressureB = NearDensityToPressure(densityB);
    return (pressureA + pressureB) / 2.0f;
}


glm::vec3 Fluid::GetRandomDirection3D() {
    float x = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f; // [-1, 1]
    float y = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f; // [-1, 1]
    float z = 0.0f;

    glm::vec3 direction(x, y, z);
    return glm::normalize(direction);
}


glm::ivec3 Fluid::PositionsToCellCoord(glm::vec3 point, float radius) {
    return glm::ivec3(static_cast<int>(std::floor(point.x / radius)),
        static_cast<int>(std::floor(point.y / radius)), 0);
}


unsigned int Fluid::HashCell(int x, int y) {
    const unsigned int p1 = 73856093;
    const unsigned int p2 = 19349663;
    return ((unsigned int) x * p1) ^ ((unsigned int) y * p2);
}


unsigned int Fluid::GetKeyFromHash(unsigned int hash) {
    return hash % _hashSize;
}


void Fluid::UpdateSpatialLookup(float radius) {
    const size_t count = _predictedPositions.size();
    std::fill(_startIndices.begin(), _startIndices.end(), MAX_INT);

    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(count); ++i) {
        glm::vec3 cellCoord = PositionsToCellCoord(_predictedPositions[i], _smoothingRadius);
        unsigned int hash = HashCell(static_cast<int>(cellCoord.x), static_cast<int>(cellCoord.y));
        unsigned int key = GetKeyFromHash(hash);

        _spatialLookup[i] = Entry(i, key);
    }

    std::sort(std::execution::par, _spatialLookup.begin(), _spatialLookup.end());

    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(count); ++i) {
        unsigned int key = _spatialLookup[i].key;
        unsigned int keyPrev = (i == 0) ? MAX_INT : _spatialLookup[i - 1].key;

        if (key != keyPrev) {
            _startIndices[key] = i;
        }
    }
}


float Fluid::ViscosityKernel(float radius, float distance) {
    if (distance > radius) return 0.0f;
    
	float volume = PI * _radius8 / 6.0f;
	float value = std::max(0.0f, _radius2 - distance * distance);
	return value * value * value / volume;
}


float Fluid::ViscosityKernelDerivative(float radius, float distance) {
    if (distance > radius) return 0.0f;

    float scale = 12.0f / (PI * _radius4);
    return (distance - radius) * scale;
}


glm::vec3 Fluid::CalculateViscosityForce(int i) {
    glm::vec3 viscosityForce(0.0f);
    glm::vec3 cellCoord = PositionsToCellCoord(_predictedPositions[i], _smoothingRadius);
    int centerX = cellCoord.x;
    int centerY = cellCoord.y;
    float sqrRadius = _smoothingRadius * _smoothingRadius;
    for (int k = 0; k < 9; ++k) {
        int offsetX = cellOffsets[k].first;
        int offsetY = cellOffsets[k].second;
        unsigned int key = GetKeyFromHash(HashCell(centerX + offsetX, centerY + offsetY));
        int cellStartIndex = _startIndices[key];
        if (cellStartIndex == MAX_INT) continue;
        for (int j = cellStartIndex; j < _spatialLookup.size(); ++j) {
            if (_spatialLookup[j].key != key) break;
            int particleIndex = _spatialLookup[j].index;
            if (particleIndex == i) continue;

            glm::vec3 offset = _predictedPositions[particleIndex] - _predictedPositions[i];
            float sqrDistance = glm::dot(offset, offset);
            if (sqrDistance < sqrRadius) {
                float distance = std::sqrt(sqrDistance);
				float influence = ViscosityKernel(_smoothingRadius, distance);
                viscosityForce += (_velocities[particleIndex] - _velocities[i]) * influence;
            }
        }
    }

	return viscosityForce * _viscosityStrength;
}


float Fluid::NearSmoothingKernel(float radius, float distance) {
    if (distance > radius) return 0.0f;

    float v = radius - distance;
    return v * v * v * _spikyKernelPow3Factor;
}


float Fluid::NearSmoothingKernelDerivative(float radius, float distance) {
    if (distance > radius) return 0.0f;

    float v = radius - distance;
    return -v * v * _derivativeSpikyPow3Factor;
}


void Fluid::ApplyInteractionForce(glm::vec2 inputPos, float radius, float strength) {
    glm::vec3 inputPos3D(inputPos, 0.0f); // extend to 3D for consistency
    glm::vec3 cellCoord = PositionsToCellCoord(inputPos3D, _smoothingRadius);
    int centerX = static_cast<int>(cellCoord.x);
    int centerY = static_cast<int>(cellCoord.y);

    float sqrRadius = radius * radius;

    for (int k = 0; k < 9; ++k) {
        int offsetX = cellOffsets[k].first;
        int offsetY = cellOffsets[k].second;

        unsigned int key = GetKeyFromHash(HashCell(centerX + offsetX, centerY + offsetY));
        int cellStartIndex = _startIndices[key];
        if (cellStartIndex == MAX_INT) continue;

        for (int j = cellStartIndex; j < _spatialLookup.size(); ++j) {
            if (_spatialLookup[j].key != key) break;

            int particleIndex = _spatialLookup[j].index;
            glm::vec2 offset = inputPos - glm::vec2(_positions[particleIndex]);  // 2D offset
            float sqrDist = glm::dot(offset, offset);
            if (sqrDist < sqrRadius) {
                float dist = sqrt(sqrDist);
                glm::vec2 direction = (dist <= EPSILON) ? glm::vec2(0.0f) : offset / dist;
                float influence = 1.0f - dist / radius;
                glm::vec2 force2D = (direction * strength - glm::vec2(_velocities[particleIndex])) * influence;

                // Apply force to X and Y components
                _velocities[particleIndex].x += force2D.x;
                _velocities[particleIndex].y += force2D.y;
            }
        }
    }
}


void Fluid::GetParticlePositions(std::vector<glm::vec3>& outPositions) {
    outPositions.clear();
    outPositions = _positions;
}

void Fluid::GetParticleVelocities(std::vector<glm::vec3>& outVelocities) {
    outVelocities.clear();
    outVelocities = _velocities;
}

float Fluid::GetPressureMultiplier() { return _pressureMultiplier; }
void Fluid::SetPressureMultiplier(float pressureMultiplier) { _pressureMultiplier = pressureMultiplier; }

float Fluid::GetTargetDensity() { return _targetDensity; }
void Fluid::SetTargetDensity(float targetDensity) { _targetDensity = targetDensity; }

void Fluid::SetGravity(float g) { _gravityAcceleration = g; }
float Fluid::GetGravity() {return _gravityAcceleration;}

void Fluid::SetPaused(bool isPaused) { _isPaused = isPaused; }

float Fluid::GetViscosityStrength() { return _viscosityStrength; }
void Fluid::SetViscosityStrength(float viscosityStrength) { _viscosityStrength = viscosityStrength; }

float Fluid::GetNearDensityMultiplier() { return _nearDensityMultiplier; }
void Fluid::SetNearDensityMultiplier(float nearDensityMultiplier) { _nearDensityMultiplier = nearDensityMultiplier; }







