#include "Fluid.h"
#include <iostream>


Fluid::Fluid(int initialParticleCount, int mergeSplitCoeff, int cooldown_frames, float delta_time, float particleRadius, float mass, float gravityAcceleration, float collisionDamping, float spacing, float pressureMultiplier, float targetDensity, float smoothingRadius, int hashSize, float interactionRadius, float interactionStrength, float viscosityStrength, float nearDensityMultiplier, float boundaryX, float boundaryY, float boundaryZ, float high_density_factor, float low_density_factor, float max_mass_factor, float min_mass_factor)
    : _initialParticleCount(initialParticleCount)
    , _mergeSplitCoeff(mergeSplitCoeff)
    , _cooldown_frames(cooldown_frames)
    , _hashSize(hashSize)
    , _particleRadius(particleRadius)
    , _mass(mass)
    , _gravityAcceleration(gravityAcceleration)
    , _collisionDamping(collisionDamping)
    , _spacing(spacing)
    , _pressureMultiplier(pressureMultiplier)
    , _targetDensity(targetDensity)
    , _smoothingRadius(smoothingRadius)
    , _interactionRadius(interactionRadius)
    , _interactionStrength(interactionStrength)
    , _viscosityStrength(viscosityStrength)
    , _nearDensityMultiplier(nearDensityMultiplier)
    , _boundaryX(boundaryX), _boundaryY(boundaryY), _boundaryZ(boundaryZ)
    , _high_density_factor(high_density_factor)
    , _low_density_factor(low_density_factor)
    , _max_mass_factor(max_mass_factor)
    , _min_mass_factor(min_mass_factor)
    // ---- derived ----
    , _maxParticleCount(_initialParticleCount* _mergeSplitCoeff)
    , _lookupCapacity(nextPowerOfTwo(_maxParticleCount))
    // ---- buffers using the derived values ----
    , _particleVectors(_maxParticleCount, GL_DYNAMIC_DRAW)
    , _particleValues(_maxParticleCount, GL_DYNAMIC_DRAW)
    , _newParticleVectors(_maxParticleCount, GL_DYNAMIC_DRAW)
    , _newParticleValues(_maxParticleCount, GL_DYNAMIC_DRAW)
    , _spatialLookup(_lookupCapacity, GL_DYNAMIC_DRAW)
    , _startIndices(_hashSize, GL_DYNAMIC_DRAW)
    , _simParams(1, GL_DYNAMIC_DRAW)
    // ---- shaders ----
    , _predictedPosShader("predicted_positions.comp")
    , _densityStep("density_step.comp")
    , _forceStep("force_step.comp")
    , _fluidStep("fluid_step.comp")
    , _bitonicSortShader("bitonic_sort.comp")
    , _updateSpatialLookup("update_spatial_lookup.comp")
    , _buildStartIndices("build_start_indices.comp")
    , _tagParticles("tag_particles.comp")
    , _resampleParticles("resample_particles.comp")
    , _resetMergeFlags("reset_merge_flags.comp")
{
	// Initialize simulation parameters
    _params = {};
    _params.dt = delta_time;
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

	_params.high_density_factor = high_density_factor;
	_params.low_density_factor = low_density_factor;
	_params.max_mass_factor = max_mass_factor;
	_params.min_mass_factor = min_mass_factor;

	_params.cooldown_frames = cooldown_frames;

	_params.mergeSplitCoefficient = mergeSplitCoeff;
	_params.initialParticleCount = initialParticleCount;
	_params.currentParticleCount = initialParticleCount;
	_params.maxParticleCount = initialParticleCount * mergeSplitCoeff;
	_params.lookupCapacity = nextPowerOfTwo(_params.maxParticleCount);
	_params.paddedCurrentParticleCount = std::min(nextPowerOfTwo(_params.currentParticleCount),_params.lookupCapacity);

	_params.hashSize = hashSize;
	_params.spacing = spacing;
	_params.particleRadius = particleRadius;
	_params.boundaryX = boundaryX;
	_params.boundaryY = boundaryY;
	_params.boundaryZ = boundaryZ;

	_params.padding1 = 0.0f;
	_params.padding2 = 0.0f;
	_params.padding3 = 0.0f;

    _simParams.upload(std::vector<SimulationParameters>{_params});

	int maxCapacity = initialParticleCount * mergeSplitCoeff;

	// Initialize all of them zero first, then set the first 'particleCount' elements
    std::vector<ParticleVectors> vectorData(maxCapacity, ParticleVectors{});
    std::vector<ParticleValues>  valueData(maxCapacity, ParticleValues{});

    std::vector<ParticleVectors> newVectorData(maxCapacity, ParticleVectors{});
    std::vector<ParticleValues>  newValueData(maxCapacity, ParticleValues{});

    const uint32_t count = _params.currentParticleCount;
    if (count == 0u) { /* handle empty */ }

    const uint32_t perAxis = static_cast<uint32_t>(
        std::ceil(std::cbrt(static_cast<double>(count)))
        );
    const uint32_t perAxis2 = perAxis * perAxis;

    for (uint32_t i = 0; i < count; ++i) {
        const uint32_t z = i / perAxis2;
        const uint32_t y = (i / perAxis) % perAxis;
        const uint32_t x = i % perAxis;

        const float fx = (static_cast<float>(x) - 0.5f * static_cast<float>(perAxis) + 0.5f) * _spacing;
        const float fy = (static_cast<float>(y) - 0.5f * static_cast<float>(perAxis) + 0.5f) * _spacing;
        const float fz = (static_cast<float>(z) - 0.5f * static_cast<float>(perAxis) + 0.5f) * _spacing;

		vectorData[i].position = glm::vec4(fx, fy, fz, 1.0f);
		vectorData[i].predictedPosition = glm::vec4(fx, fy, fz, 1.0f);
        vectorData[i].velocity = glm::vec4(0.0f);
        valueData[i].mass = mass;
        valueData[i].density = 0.0f;
        valueData[i].nearDensity = 0.0f;
		valueData[i].particleRadius = particleRadius;
		valueData[i].tag = 0;
		valueData[i].mergeFlag = 0;
        valueData[i].padding = 0.0f;

        newVectorData[i].position = glm::vec4(fx, fy, fz, 1.0f);
        newVectorData[i].predictedPosition = glm::vec4(fx, fy, fz, 1.0f);
        newVectorData[i].velocity = glm::vec4(0.0f);
        newValueData[i].mass = mass;
        newValueData[i].density = 0.0f;
        newValueData[i].nearDensity = 0.0f;
        newValueData[i].particleRadius = particleRadius;
        newValueData[i].tag = 0;
        newValueData[i].mergeFlag = 0;
        newValueData[i].padding = 0.0f;
    }

	_particleVectors.upload(vectorData);
	_particleValues.upload(valueData);
	_newParticleVectors.upload(newVectorData);
	_newParticleValues.upload(newValueData);
    
    std::vector<Entry> lookupData(_params.lookupCapacity);

    for (size_t i = 0; i < _params.currentParticleCount; ++i) {
        lookupData[i].index = 0u;
        lookupData[i].key = 0u;          // placeholder
        lookupData[i].padding1 = lookupData[i].padding2 = 0.0f;
    }
    for (size_t i = _params.currentParticleCount; i < _params.lookupCapacity; ++i) {
        lookupData[i].index = -1;
        lookupData[i].key = 0xFFFFFFFFu; // always sorts to the back
        lookupData[i].padding1 = lookupData[i].padding2 = 0.0f;
    }

    _spatialLookup.upload(lookupData);
    _startIndices.upload(std::vector<int>(hashSize, MAX_INT));

    GLuint zero = 0;
    glGenBuffers(1, &_newParticleCounterBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _newParticleCounterBuffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

void Fluid::Update(float dt) {
	if (_params.isPaused) return; // Skip update if paused

    _simParams.upload(std::vector<SimulationParameters>{_params});
    

    const int groupSize = 256;
    int oldCount = _params.currentParticleCount;
    int oldNumGroups = (oldCount + groupSize - 1) / groupSize;

	// Step 0: Predict positions based on velocities
	_predictedPosShader.use();
	_particleVectors.bindTo(0);
	_simParams.bindTo(6);
    _predictedPosShader.dispatch(oldNumGroups);
	_predictedPosShader.wait();

    // Step 1: Update spatial hashing
	// !!! MAIN OPTIMIZATION BOTTLENECK IS HERE, WE ARE SORTING TWO TIMES !!!
	UpdateSpatialHashing(oldNumGroups);

	// Step 2: Calculate densities

	// !!! WE MIGHT HAVE PROBLEM HERE THAT LEADS PERFORMANCE DROP OVER TIME !!!
	_densityStep.use();
	_particleVectors.bindTo(0);
	_particleValues.bindTo(1);
	_spatialLookup.bindTo(4);
	_startIndices.bindTo(5);
	_simParams.bindTo(6);
	_densityStep.dispatch(oldNumGroups);
	_densityStep.wait();

    // Step 3: Adaptive sampling
    // 3a) Tag each particle KEEP/SPLIT/MERGE
    _tagParticles.use();
    _particleValues.bindTo(1);
    _simParams.bindTo(6);
    _tagParticles.dispatch(oldNumGroups);
    _tagParticles.wait();

    // 3b) Reset atomic counter and merge flags
	_resetMergeFlags.use();
	_particleValues.bindTo(1);
	_simParams.bindTo(6);
	_resetMergeFlags.dispatch(oldNumGroups);
	_resetMergeFlags.wait();

    resetParticleCounter();

    // 3c) Run resampling: write KEEP/SPLIT/MERGE into newVec/ValueData
	// !!! TOO EXPANSIVE OPERATION, OPTIMIZE IT LATER !!!
    _resampleParticles.use();
    _particleVectors.bindTo(0);
    _particleValues.bindTo(1);
    _newParticleVectors.bindTo(2);
    _newParticleValues.bindTo(3);
    _spatialLookup.bindTo(4);
    _startIndices.bindTo(5);
    _simParams.bindTo(6);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 7, _newParticleCounterBuffer);

    _resampleParticles.dispatch(oldNumGroups);
    _resampleParticles.wait();

    // 3d) Read back the new particle count
    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _newParticleCounterBuffer);
    GLuint* counterPtr = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
    GLuint newCount = *counterPtr;
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    // 3e) Update our C++ state and GPU sim‐params
    _params.currentParticleCount = newCount;
    // std::cout << "RESAMPLED COUNT = " << newCount << "\n";
    _params.paddedCurrentParticleCount = nextPowerOfTwo(newCount);
    _simParams.upload({ _params });

    int newNumGroups = (newCount + groupSize - 1) / groupSize;

    // 3f) Swap in the new buffers so the rest of the pipeline uses them
    GLuint a = _particleVectors.getID();
    GLuint b = _newParticleVectors.getID();
    _particleVectors.setID(b);
    _newParticleVectors.setID(a);

    GLuint c = _particleValues.getID();
    GLuint d = _newParticleValues.getID();
    _particleValues.setID(d);
    _newParticleValues.setID(c);

    BindRenderBuffers();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    /*std::vector<glm::vec4> snapshot(10);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _particleVectors.getID());
    glGetBufferSubData(
        GL_SHADER_STORAGE_BUFFER,
        0,
        sizeof(glm::vec4) * snapshot.size(),
        snapshot.data()
    );
    for (int i = 0; i < (int)snapshot.size(); ++i) {
        auto& p = snapshot[i];
        std::cout << "pos[" << i << "] = "
            << p.x << "," << p.y << "," << p.z << "\n";
    }*/

	// Step 4: Update spatial lookup with new particle count
	UpdateSpatialHashing(newNumGroups);


    // Step 5: Again calculate densities
	// !!! OPTIMIZATION PROBLEM, DO WE REALLY NEED TO CALCULATE DENSITIES TWICE???
    _densityStep.use();
    _particleVectors.bindTo(0);
    _particleValues.bindTo(1);
    _spatialLookup.bindTo(4);
    _startIndices.bindTo(5);
    _simParams.bindTo(6);
    _densityStep.dispatch(newNumGroups);
    _densityStep.wait();

	// Step 6: Calculate forces with new particle count
	// !!! LOOKS LIKE WE HAVE OPTIMIZATION PROBLEMS HERE !!! THE MOST EXPENSIVE STEP !!! WHY???
	_forceStep.use();
	_particleVectors.bindTo(0);
	_particleValues.bindTo(1);
	_spatialLookup.bindTo(4);
	_startIndices.bindTo(5);
	_simParams.bindTo(6);
	_forceStep.dispatch(newNumGroups);
	_forceStep.wait();

	// Step 7: Update positions and velocities with new particle count
	_fluidStep.use();
    _particleVectors.bindTo(0);
    _simParams.bindTo(6);
    _fluidStep.dispatch(newNumGroups);
    _fluidStep.wait();
}

void Fluid::resetParticleCounter() {
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _newParticleCounterBuffer);
    GLuint zero = 0;
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 7, _newParticleCounterBuffer);
    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);
}


void Fluid::UpdateSpatialHashing(int numGroups) {
    // Step 1: Update spatial lookup keys
    _updateSpatialLookup.use();
    _particleVectors.bindTo(0);
    _spatialLookup.bindTo(4);
    _simParams.bindTo(6);
    _updateSpatialLookup.dispatch(numGroups);
    _updateSpatialLookup.wait();

    // Step 2: Sort spatial lookup
    SortSpatialLookup();

    // Step 3: Clear start indices
    _startIndices.upload(std::vector<int>(_params.hashSize, MAX_INT));

    // Step 4: Update start indices
    _buildStartIndices.use();
    _spatialLookup.bindTo(4);
    _startIndices.bindTo(5);
    _simParams.bindTo(6);
    _buildStartIndices.dispatch(numGroups);
    _buildStartIndices.wait();
}

GLuint Fluid::nextPowerOfTwo(GLuint x) {
    GLuint p = 1;
    while (p < x) p <<= 1;
    return p;
}

void Fluid::SortSpatialLookup() {
    const GLuint actualN = _params.currentParticleCount;
    const GLuint paddedN = nextPowerOfTwo(actualN);
    const GLuint localSize = 256;
    const GLuint numGroups = (paddedN + localSize - 1) / localSize;

    _bitonicSortShader.use();
    _spatialLookup.bindTo(4);

    _bitonicSortShader.setUint("u_N", paddedN);
    for (GLuint size = 2; size <= paddedN; size <<= 1) {
        for (GLuint stride = size >> 1; stride > 0; stride >>= 1) {
            _bitonicSortShader.setUint("u_size", size);
            _bitonicSortShader.setUint("u_stride", stride);
            _bitonicSortShader.dispatch(numGroups);
            _bitonicSortShader.wait();
        }
    }
}

void Fluid::BindRenderBuffers() {
	_particleVectors.bindTo(0);
	_particleValues.bindTo(1);
}

void Fluid::SetIsInteracting(bool state) { _params.isInteracting = state; }
void Fluid::SetInteractionPosition(glm::vec3 pos) { 
    _params.inputPositionX = pos.x;
	_params.inputPositionY = pos.y;
	_params.inputPositionZ = pos.z;
}
void Fluid::SetInteractionStrength(float strength) { _params.interactionStrength = strength; }
void Fluid::SetInteractionRadius(float radius) { _params.interactionRadius = radius; }

float Fluid::GetPressureMultiplier() const { return _params.pressureMultiplier; }
void Fluid::SetPressureMultiplier(float pressureMultiplier) { _params.pressureMultiplier = pressureMultiplier; }

float Fluid::GetTargetDensity() const { return _params.targetDensity; }
void Fluid::SetTargetDensity(float targetDensity) { _params.targetDensity = targetDensity; }

float Fluid::GetGravity() const { return _params.gravityAcceleration; }
void Fluid::SetGravity(float g) { _params.gravityAcceleration = g; }

void Fluid::SetPaused(bool isPaused) { _params.isPaused = isPaused; }

float Fluid::GetViscosityStrength() const { return _params.viscosityStrength; }
void Fluid::SetViscosityStrength(float viscosityStrength) { _params.viscosityStrength = viscosityStrength; }

float Fluid::GetNearDensityMultiplier() const { return _params.nearDensityMultiplier; }
void Fluid::SetNearDensityMultiplier(float nearDensityMultiplier) { _params.nearDensityMultiplier = nearDensityMultiplier; }

int Fluid::GetParticleCount() const { return _params.currentParticleCount; }







