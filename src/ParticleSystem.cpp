#include "ParticleSystem.h"
#include <iostream>

ParticleSystem::ParticleSystem() {}

ParticleSystem::~ParticleSystem() {
    cleanup();
}

void ParticleSystem::init(const std::vector<Particle>& initialParticles,
                          const std::vector<EmitterConfig>& emitters,
                          const std::string& shaderDir) {
    maxParticles = static_cast<uint32_t>(initialParticles.size());
    activeParticles = maxParticles;
    numEmitters = static_cast<uint32_t>(emitters.size());

    createSSBO(initialParticles);
    createEmitterSSBO(emitters);
    setupVAO();
    loadShaders(shaderDir);

    glGenBuffers(1, &atomicBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);

    GLuint zero = 0;
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicBuffer);

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

void ParticleSystem::cleanup() {
    if (particleSSBO) {
        glDeleteBuffers(1, &particleSSBO);
    }

    if (emitterSSBO) {
        glDeleteBuffers(1, &emitterSSBO);
    }

    if (atomicBuffer) {
        glDeleteBuffers(1, &atomicBuffer);
    }

    if (particleVAO) {
        glDeleteVertexArrays(1, &particleVAO);
    }

    particleSSBO = 0;
    emitterSSBO = 0;
    atomicBuffer = 0;
    particleVAO = 0;
}

void ParticleSystem::createSSBO(const std::vector<Particle>& particles) {
    glGenBuffers(1, &particleSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);

    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        particles.size() * sizeof(Particle),
        particles.data(),
        GL_DYNAMIC_DRAW
    );

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ParticleSystem::createEmitterSSBO(const std::vector<EmitterConfig>& emitters) {
    std::vector<GPUEmitter> gpuEmitters;
    gpuEmitters.reserve(emitters.size());

    for (const auto& em : emitters) {
        GPUEmitter gpuEm{};

        gpuEm.positionAndShape = glm::vec4(
            em.position,
            static_cast<float>(em.shape)
        );

        gpuEm.directionAndRate = glm::vec4(
            em.direction,
            em.emitRate
        );

        gpuEm.dimensions = glm::vec4(
            em.width,
            em.height,
            em.radius,
            em.particleLife
        );

        gpuEm.speedAndTemp = glm::vec4(
            em.initialSpeed,
            em.speedVariance,
            em.temperature,
            em.particleSize
        );

        gpuEm.timing = glm::vec4(
            em.startTime,
            em.endTime,
            em.fadeIn,
            em.fadeOut
        );

        gpuEmitters.push_back(gpuEm);
    }

    glGenBuffers(1, &emitterSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, emitterSSBO);

    size_t bufferSize = gpuEmitters.empty()
        ? sizeof(GPUEmitter)
        : gpuEmitters.size() * sizeof(GPUEmitter);

    const void* data = gpuEmitters.empty()
        ? nullptr
        : gpuEmitters.data();

    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        bufferSize,
        data,
        GL_STATIC_DRAW
    );

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, emitterSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ParticleSystem::setupVAO() {
    glGenVertexArrays(1, &particleVAO);
    glBindVertexArray(particleVAO);
    glBindVertexArray(0);
}

void ParticleSystem::loadShaders(const std::string& shaderDir) {
    std::string vert = shaderDir + "/particle.vert";
    std::string geom = shaderDir + "/particle.geom";
    std::string frag = shaderDir + "/particle.frag";

    if (!renderShader.loadGraphics(vert, frag, geom)) {
        std::cerr << "Fallo al cargar shaders de renderizado de partículas.\n";
    }

    if (!updateShader.loadCompute(shaderDir + "/particle_update.comp")) {
        std::cerr << "Fallo al cargar shader compute particle_update.comp\n";
    }

    if (!emitShader.loadCompute(shaderDir + "/particle_emit.comp")) {
        std::cerr << "Fallo al cargar shader compute particle_emit.comp\n";
    }
}

void ParticleSystem::setCurlNoiseParams(const CurlNoiseParams& params) {
    curlParams = params;
}

void ParticleSystem::setWind(const glm::vec3& dir, float strength) {
    if (glm::length(dir) > 0.0001f) {
        windDirection = glm::normalize(dir);
    } else {
        windDirection = glm::vec3(0.0f);
    }

    windStrength = strength;
}

void ParticleSystem::setComputeUniforms(Shader& shader, float deltaTime, float currentTime) {
    shader.use();

    // Nombres usados por particle_update.comp actual.
    shader.setFloat("deltaTime", deltaTime);
    shader.setFloat("currentTime", currentTime);
    shader.setVec3("gravity", gravity);
    shader.setFloat("buoyancyStrength", buoyancyStrength);
    shader.setVec3("windDirection", windDirection);
    shader.setFloat("windStrength", windStrength);

    // Nombres alternativos por si algún shader viejo sigue con prefijo u_.
    shader.setFloat("u_deltaTime", deltaTime);
    shader.setFloat("u_time", currentTime);
    shader.setUint("u_maxParticles", maxParticles);
    shader.setVec3("u_gravity", gravity);
    shader.setFloat("u_buoyancy", buoyancyStrength);
    shader.setVec3("u_windDir", windDirection);
    shader.setFloat("u_windStrength", windStrength);

    // Uniforms reales de curl_noise.glsl.
    shader.setFloat("u_curlFrequency", curlParams.frequency);
    shader.setFloat("u_curlAmplitude", curlParams.amplitude);
    shader.setInt("u_curlOctaves", curlParams.octaves);
    shader.setFloat("u_curlLacunarity", curlParams.lacunarity);
    shader.setFloat("u_curlPersistence", curlParams.persistence);
    shader.setFloat("u_curlTimeScale", curlParams.timeScale);
    shader.setFloat("u_curlEpsilon", curlParams.epsilon);

    // Alias viejos por compatibilidad.
    shader.setFloat("u_curlFreq", curlParams.frequency);
    shader.setFloat("u_curlAmp", curlParams.amplitude);
}

void ParticleSystem::dispatchUpdateCompute(float deltaTime, float currentTime) {
    if (maxParticles == 0) {
        return;
    }

    setComputeUniforms(updateShader, deltaTime, currentTime);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);

    GLuint numGroups = (maxParticles + 255) / 256;

    glDispatchCompute(numGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
}

void ParticleSystem::dispatchEmitCompute(float deltaTime, float currentTime) {
    if (numEmitters == 0 || maxParticles == 0) {
        return;
    }

    emitShader.use();

    // Para el particle_emit.comp nuevo.
    emitShader.setFloat("u_deltaTime", deltaTime);
    emitShader.setFloat("u_time", currentTime);
    emitShader.setUint("u_maxParticles", maxParticles);
    emitShader.setUint("u_numEmitters", numEmitters);

    // Para el particle_emit.comp viejo.
    emitShader.setFloat("deltaTime", deltaTime);
    emitShader.setFloat("currentTime", currentTime);
    emitShader.setUint("numEmitters", numEmitters);

    // Emitimos como máximo una parte del pool por frame.
    // Si esto queda muy bajo, no aparece fuego; si queda muy alto, explota la escena.
    GLuint maxEmitThisFrame = std::max<GLuint>(256, maxParticles / 20);
    emitShader.setUint("maxParticlesToEmitThisFrame", maxEmitThisFrame);

    // Escala de emisión variable en el tiempo (fuego que crece y se apaga).
    emitShader.setFloat("u_emitScale", emitScale);

    // Reset atomic counter para shaders que usan atomic_uint.
    if (atomicBuffer) {
        GLuint zero = 0;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
        glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicBuffer);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, emitterSSBO);

    // En vez de numEmitters, recorremos todo el pool.
    // Así cada hilo puede intentar reciclar una partícula muerta.
    GLuint numGroups = (maxParticles + 255) / 256;

    glDispatchCompute(numGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
}

void ParticleSystem::update(float deltaTime, float currentTime) {
    dispatchUpdateCompute(deltaTime, currentTime);
    dispatchEmitCompute(deltaTime, currentTime);
}

void ParticleSystem::render(const Camera& camera, float aspectRatio) {
    if (maxParticles == 0) {
        return;
    }

    renderShader.use();

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(aspectRatio);
    glm::vec3 camRight = camera.getRight();
    glm::vec3 camUp = camera.getUp();

    // Nombres reales usados por particle.geom.
    renderShader.setMat4("view", view);
    renderShader.setMat4("projection", proj);
    renderShader.setVec3("cameraRight", camRight);
    renderShader.setVec3("cameraUp", camUp);

    // Alias viejos por compatibilidad.
    renderShader.setMat4("u_view", view);
    renderShader.setMat4("u_projection", proj);
    renderShader.setVec3("u_cameraRight", camRight);
    renderShader.setVec3("u_cameraUp", camUp);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);

    glBindVertexArray(particleVAO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    renderShader.setInt("renderPass", 0);
    glDrawArrays(GL_POINTS, 0, maxParticles);

    glDepthMask(GL_FALSE);
    renderShader.setInt("renderPass", 1);
    glDrawArrays(GL_POINTS, 0, maxParticles);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glBindVertexArray(0);
}
