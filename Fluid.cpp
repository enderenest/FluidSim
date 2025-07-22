#include "Fluid.h"
#include <iostream>


Fluid::Fluid(const unsigned int particleCount, const float particleRadius, const float mass, const float gravityAcceleration, const float collisionDamping, const float spacing, const float pressureMultiplier, const float targetDensity, const float smoothingRadius, const unsigned int hashSize, const float interactionRadius, const float interactionStrength, float viscosityStrength, float nearDensityMultiplier, float boundaryX, float boundaryY, float boundaryZ)
    : _positions(particleCount, GL_DYNAMIC_DRAW),
      _predictedPositions(particleCount, GL_DYNAMIC_DRAW),
      _velocities(particleCount, GL_DYNAMIC_DRAW),
      _densities(particleCount, GL_DYNAMIC_DRAW),
      _nearDensities(particleCount, GL_DYNAMIC_DRAW),
	  _spatialLookup(particleCount, GL_DYNAMIC_DRAW),
      _startIndices(hashSize, GL_DYNAMIC_DRAW),
      _simParams(1, GL_STATIC_DRAW),

      _predictedPosShader("predicted_positions.comp"),
	  _densityStep("density_step.comp"),
	  _forceStep("force_step.comp"),
      _fluidStep("fluid_step.comp"),
	  _bitonicSortShader("bitonic_sort.comp"),
	  _updateSpatialLookup("update_spatial_lookup.comp"),
	  _buildStartIndices("build_start_indices.comp")
	  
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
    _params.inputPositionX = 0.0f;
    _params.inputPositionY = 0.0f;
    _params.inputPositionZ = 0.0f;
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
    std::vector<glm::vec4> initialPositions(_params.particleCount, glm::vec4(0.0f));

    int particlesPerRow = static_cast<int>(std::sqrt(particleCount));
    int particlesPerCol = (particleCount - 1) / particlesPerRow + 1;

    for (unsigned int i = 0; i < particleCount; ++i) {
        int row = i / particlesPerRow;
        int col = i % particlesPerRow;

        float x = (col - particlesPerRow / 2.0f + 0.5f) * spacing;
        float y = (row - particlesPerCol / 2.0f + 0.5f) * spacing;

		initialPositions[i] = glm::vec4(x, y, 0.0f, 0.0f);
    }

    _positions.upload(initialPositions);
    _predictedPositions.upload(initialPositions);
    _velocities.upload(std::vector<glm::vec4>(particleCount, glm::vec4(0.0f)));
    _densities.upload(std::vector<float>(particleCount, 0.0f));
    _nearDensities.upload(std::vector<float>(particleCount, 0.0f));
	_spatialLookup.upload(std::vector<Entry>(particleCount, Entry{ 0, 0 }));
    _startIndices.upload(std::vector<unsigned int>(hashSize, MAX_INT));
}

void Fluid::Update(float dt) {
	if (_params.isPaused) return; // Skip update if paused

    _simParams.upload(std::vector<SimulationParameters>{_params});
    

    const int groupSize = 256;
    int numGroups = (_positions.count() + groupSize - 1) / groupSize;

	// Step 0: Predict positions based on velocities
	_predictedPosShader.use();
	_positions.bindTo(1);
	_predictedPositions.bindTo(2);
	_velocities.bindTo(3);
	_simParams.bindTo(8);
    _predictedPosShader.dispatch(numGroups);
	_predictedPosShader.wait();

    // Step 1: Update spatial lookup keys,
	_spatialLookup.upload(std::vector<Entry>(_params.particleCount, Entry{ 0, 0 }));

    _updateSpatialLookup.use();
	_predictedPositions.bindTo(2);
    _spatialLookup.bindTo(6);
    _simParams.bindTo(8);
    _updateSpatialLookup.dispatch(numGroups);
	_updateSpatialLookup.wait();

    // Step 2: Sort spatial lookup
    SortSpatialLookup();

	// Step 3: Clear start indices
    _startIndices.upload(std::vector<unsigned int>(_params.hashSize, MAX_INT));
 
	// Step 4: Update start indices
    _buildStartIndices.use();
    _spatialLookup.bindTo(6);
    _startIndices.bindTo(7);
    _simParams.bindTo(8);
    _buildStartIndices.dispatch(numGroups);
	_buildStartIndices.wait();

	// Step 5: Calculate densities
    _densities.upload(std::vector<float>(_params.particleCount, 0.0f));
    _nearDensities.upload(std::vector<float>(_params.particleCount, 0.0f));

	_densityStep.use();
	_predictedPositions.bindTo(2);
	_densities.bindTo(4);
	_nearDensities.bindTo(5);
	_spatialLookup.bindTo(6);
	_startIndices.bindTo(7);
	_simParams.bindTo(8);
	_densityStep.dispatch(numGroups);
	_densityStep.wait();

	// Step 6: Calculate forces
	_forceStep.use();
	_positions.bindTo(1);
	_predictedPositions.bindTo(2);
	_velocities.bindTo(3);
	_densities.bindTo(4);
	_nearDensities.bindTo(5);
	_spatialLookup.bindTo(6);
	_startIndices.bindTo(7);
	_simParams.bindTo(8);
	_forceStep.dispatch(numGroups);
	_forceStep.wait();

	// Step 7: Update positions and velocities
	_fluidStep.use();
    _positions.bindTo(1);
    _velocities.bindTo(3);
    _simParams.bindTo(8);
    _fluidStep.dispatch(numGroups);
    _fluidStep.wait();
}


void Fluid::SortSpatialLookup() {
    GLuint N = _params.particleCount;
    GLuint localSize = 256;
    const GLuint groups = (N + localSize - 1) / localSize;       

    _bitonicSortShader.use();
    _spatialLookup.bindTo(6);

    _bitonicSortShader.setUint("u_N", N);

    for (GLuint size = 2; size <= N; size <<= 1) {
        for (GLuint stride = size >> 1; stride > 0; stride >>= 1) {
            _bitonicSortShader.setUint("u_size", size);
            _bitonicSortShader.setUint("u_stride", stride);

            // ← dispatch here, not after the loops
            _bitonicSortShader.dispatch(groups, 1, 1);
            _bitonicSortShader.wait();
        }
    }
}


void Fluid::BindRenderBuffers() {
    _positions.bindTo(1);
    _velocities.bindTo(3);
}

void Fluid::SetIsInteracting(bool state) { _params.isInteracting = state; }
void Fluid::SetInteractionPosition(glm::vec3 pos) { 
    _params.inputPositionX = pos.x;
	_params.inputPositionY = pos.y;
	_params.inputPositionZ = pos.z;
}
void Fluid::SetInteractionStrength(float strength) { _params.interactionStrength = strength; }
void Fluid::SetInteractionRadius(float radius) { _params.interactionRadius = radius; }

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







