#include "Fluid.h"
#include <iostream>


Fluid::Fluid(const unsigned int particleCount, const float particleRadius, const float mass, const float gravityAcceleration, const float collisionDamping, const float spacing, const float pressureMultiplier, const float targetDensity, const float smoothingRadius, const unsigned int hashSize, const float interactionRadius, const float interactionStrength, float viscosityStrength, float nearDensityMultiplier, float boundaryX, float boundaryY, float boundaryZ)
    : _particleVectors(particleCount, GL_DYNAMIC_DRAW),
      _particleValues(particleCount, GL_DYNAMIC_DRAW),
      _newParticleVectors(particleCount * 2, GL_DYNAMIC_DRAW),
      _newParticleValues(particleCount * 2, GL_DYNAMIC_DRAW),
	  _spatialLookup(nextPowerOfTwo(particleCount), GL_DYNAMIC_DRAW),
      _startIndices(hashSize, GL_DYNAMIC_DRAW),
      _simParams(1, GL_DYNAMIC_DRAW),
        
      _predictedPosShader("predicted_positions.comp"),
	  _densityStep("density_step.comp"),
	  _forceStep("force_step.comp"),
      _fluidStep("fluid_step.comp"),
	  _bitonicSortShader("bitonic_sort.comp"),
	  _updateSpatialLookup("update_spatial_lookup.comp"),
	  _buildStartIndices("build_start_indices.comp"),
	  _tagParticles("tag_particles.comp"),
      _resampleParticles("resample_particles.comp"),
	  _resetMergeFlags("reset_merge_flags.comp")
{
	// Initialize simulation parameters
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
	_params.paddedParticleCount = nextPowerOfTwo(particleCount);
	_params.hashSize = hashSize;
	_params.spacing = spacing;
	_params.particleRadius = particleRadius;
	_params.boundaryX = boundaryX;
	_params.boundaryY = boundaryY;
	_params.boundaryZ = boundaryZ;

    _simParams.upload(std::vector<SimulationParameters>{_params});

	std::vector<ParticleVectors> vectorData(particleCount);
	std::vector<ParticleValues> valueData(particleCount);

    unsigned int particlesPerAxis = static_cast<unsigned int>(std::ceil(std::cbrt(particleCount)));

    for (unsigned int i = 0; i < particleCount; ++i) {
        unsigned int z = i / (particlesPerAxis * particlesPerAxis);
        unsigned int y = (i / particlesPerAxis) % particlesPerAxis;
        unsigned int x = i % particlesPerAxis;

        float fx = (static_cast<float>(x) - particlesPerAxis / 2.0f + 0.5f) * spacing;
        float fy = (static_cast<float>(y) - particlesPerAxis / 2.0f + 0.5f) * spacing;
        float fz = (static_cast<float>(z) - particlesPerAxis / 2.0f + 0.5f) * spacing;

		vectorData[i].position = glm::vec4(fx, fy, fz, 1.0f);
		vectorData[i].predictedPosition = glm::vec4(fx, fy, fz, 1.0f);
        vectorData[i].velocity = glm::vec4(0.0f);
        valueData[i].mass = mass;
        valueData[i].density = 0.0f;
        valueData[i].nearDensity = 0.0f;
		valueData[i].particleRadius = particleRadius;
		valueData[i].tag = 0;
		valueData[i].mergeFlag = 0;
        valueData[i].padding1 = valueData[i].padding2 = 0.0f;
    }

    std::vector<ParticleVectors> newVectorData(particleCount * 2);
    std::vector<ParticleValues> newValueData(particleCount * 2);

    for (unsigned int i = 0; i < particleCount; ++i) {
        unsigned int z = i / (particlesPerAxis * particlesPerAxis);
        unsigned int y = (i / particlesPerAxis) % particlesPerAxis;
        unsigned int x = i % particlesPerAxis;

        float fx = (static_cast<float>(x) - particlesPerAxis / 2.0f + 0.5f) * spacing;
        float fy = (static_cast<float>(y) - particlesPerAxis / 2.0f + 0.5f) * spacing;
        float fz = (static_cast<float>(z) - particlesPerAxis / 2.0f + 0.5f) * spacing;

        newVectorData[i].position = glm::vec4(fx, fy, fz, 1.0f);
        newVectorData[i].predictedPosition = glm::vec4(fx, fy, fz, 1.0f);
        newVectorData[i].velocity = glm::vec4(0.0f);
        newValueData[i].mass = mass;
        newValueData[i].density = 0.0f;
        newValueData[i].nearDensity = 0.0f;
        newValueData[i].particleRadius = particleRadius;
        newValueData[i].tag = 0;
        newValueData[i].mergeFlag = 0;
        newValueData[i].padding1 = newValueData[i].padding2 = 0.0f;
    }

	_particleVectors.upload(vectorData);
	_particleValues.upload(valueData);
	_newParticleVectors.upload(newVectorData);
	_newParticleValues.upload(newValueData);
    
	int paddedCount = nextPowerOfTwo(particleCount);
    std::vector<Entry> lookupData(paddedCount);

    for (unsigned i = 0; i < particleCount; ++i) {
        lookupData[i].index = 0u;
        lookupData[i].key = 0u;          // placeholder
    }
    for (unsigned i = particleCount; i < paddedCount; ++i) {
        lookupData[i].index = -1;
        lookupData[i].key = 0xFFFFFFFFu; // always sorts to the back
    }

    _spatialLookup.upload(lookupData);
    _startIndices.upload(std::vector<unsigned int>(hashSize, MAX_INT));

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
    

    const int groupSize = 512;
    int oldCount = _params.particleCount;
    int oldNumGroups = (oldCount + groupSize - 1) / groupSize;

	// Step 0: Predict positions based on velocities
	_predictedPosShader.use();
	_particleVectors.bindTo(0);
	_simParams.bindTo(6);
    _predictedPosShader.dispatch(oldNumGroups);
	_predictedPosShader.wait();

    // Step 1: Update spatial hashing
	UpdateSpatialHashing(oldNumGroups);

	// Step 2: Calculate densities
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
    _resampleParticles.use();
    _particleVectors.bindTo(0);
    _particleValues.bindTo(1);
    _spatialLookup.bindTo(4);
    _startIndices.bindTo(5);
    _newParticleVectors.bindTo(2);
    _newParticleValues.bindTo(3);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 7, _newParticleCounterBuffer);
    _simParams.bindTo(6);

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
    _params.particleCount = newCount;
    std::cout << "RESAMPLED COUNT = " << newCount << "\n";
    _params.paddedParticleCount = nextPowerOfTwo(newCount);
    _simParams.upload({ _params });

    int newNumGroups = (newCount + groupSize - 1) / groupSize;

    // 3f) Swap in the new buffers so the rest of the pipeline uses them
    GLuint a = _particleVectors.getID();
    GLuint b = _newParticleVectors.getID();
    std::swap(a, b);
    _particleVectors.setID(a);
    _newParticleVectors.setID(b);

    GLuint c = _particleValues.getID();
    GLuint d = _newParticleValues.getID();
    std::swap(c, d);
    _particleValues.setID(c);
    _newParticleValues.setID(d);

    //BindRenderBuffers();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    std::vector<glm::vec4> snapshot(10);
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
    }

	// Step 4: Update spatial lookup with new particle count
	UpdateSpatialHashing(newNumGroups);


    // Step 5: Again calculate densities
    _densityStep.use();
    _particleVectors.bindTo(0);
    _particleValues.bindTo(1);
    _spatialLookup.bindTo(4);
    _startIndices.bindTo(5);
    _simParams.bindTo(6);
    _densityStep.dispatch(newNumGroups);
    _densityStep.wait();

	// Step 6: Calculate forces with new particle count
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
    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}


void Fluid::UpdateSpatialHashing(unsigned int groups) {
    // Step 1: Update spatial lookup keys
    _updateSpatialLookup.use();
    _particleVectors.bindTo(0);
    _spatialLookup.bindTo(4);
    _simParams.bindTo(6);
    _updateSpatialLookup.dispatch(groups);
    _updateSpatialLookup.wait();

    // Step 2: Sort spatial lookup
    SortSpatialLookup();

    // Step 3: Clear start indices
    _startIndices.upload(std::vector<unsigned int>(_params.hashSize, MAX_INT));

    // Step 4: Update start indices
    _buildStartIndices.use();
    _spatialLookup.bindTo(4);
    _startIndices.bindTo(5);
    _simParams.bindTo(6);
    _buildStartIndices.dispatch(groups);
    _buildStartIndices.wait();
}

GLuint Fluid::nextPowerOfTwo(GLuint x) {
    GLuint p = 1;
    while (p < x) p <<= 1;
    return p;
}

void Fluid::SortSpatialLookup() {
    const GLuint actualN = _params.particleCount;
    const GLuint paddedN = nextPowerOfTwo(actualN);
    const GLuint localSize = 512;
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

unsigned int Fluid::GetParticleCount() const { return _params.particleCount; }







