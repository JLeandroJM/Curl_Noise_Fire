#include "ParticleSystem.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>

namespace {

struct PaperMeshVertex {
    glm::vec3 position;
    glm::vec4 color;
    float burnStart;
    glm::vec2 uv;
};

bool almostEqual(float a, float b) {
    return std::abs(a - b) < 0.00001f;
}

std::vector<float> uniqueSortedCoordinates(std::vector<float> values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end(), almostEqual), values.end());
    return values;
}

size_t nearestCoordinateIndex(const std::vector<float>& values, float value) {
    auto it = std::lower_bound(values.begin(), values.end(), value);
    if (it == values.end()) {
        return values.size() - 1;
    }
    if (it == values.begin()) {
        return 0;
    }

    size_t hi = static_cast<size_t>(it - values.begin());
    size_t lo = hi - 1;
    return std::abs(values[lo] - value) <= std::abs(values[hi] - value) ? lo : hi;
}

} // namespace

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
    createPaperMesh(initialParticles);
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

    if (paperMeshEBO) {
        glDeleteBuffers(1, &paperMeshEBO);
    }

    if (paperMeshVBO) {
        glDeleteBuffers(1, &paperMeshVBO);
    }

    if (paperMeshVAO) {
        glDeleteVertexArrays(1, &paperMeshVAO);
    }

    particleSSBO = 0;
    emitterSSBO = 0;
    atomicBuffer = 0;
    particleVAO = 0;
    paperMeshVAO = 0;
    paperMeshVBO = 0;
    paperMeshEBO = 0;
    paperMeshIndexCount = 0;
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

        gpuEm.positionAndShape = glm::vec4(em.position, static_cast<float>(em.shape));
        gpuEm.directionAndRate = glm::vec4(em.direction, em.emitRate);
        gpuEm.dimensions = glm::vec4(em.width, em.height, em.radius, em.particleLife);
        gpuEm.speedAndTemp = glm::vec4(
            em.initialSpeed,
            em.speedVariance,
            em.temperature,
            em.particleSize
        );

        gpuEmitters.push_back(gpuEm);
    }

    glGenBuffers(1, &emitterSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, emitterSSBO);

    size_t bufferSize = gpuEmitters.empty()
        ? sizeof(GPUEmitter)
        : gpuEmitters.size() * sizeof(GPUEmitter);

    const void* data = gpuEmitters.empty() ? nullptr : gpuEmitters.data();

    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, data, GL_STATIC_DRAW);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, emitterSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ParticleSystem::createPaperMesh(const std::vector<Particle>& particles) {
    std::vector<const Particle*> paperParticles;
    std::vector<float> xs;
    std::vector<float> zs;

    for (const auto& p : particles) {
        if (p.type != TYPE_PAPER) {
            continue;
        }

        paperParticles.push_back(&p);
        xs.push_back(p.position.x);
        zs.push_back(p.position.z);
    }

    if (paperParticles.size() < 4) {
        return;
    }

    xs = uniqueSortedCoordinates(std::move(xs));
    zs = uniqueSortedCoordinates(std::move(zs));

    if (xs.size() < 2 || zs.size() < 2 || xs.size() * zs.size() != paperParticles.size()) {
        std::cout << "[ParticleSystem] Paper particles are not a regular grid; mesh disabled.\n";
        return;
    }

    std::vector<PaperMeshVertex> vertices(xs.size() * zs.size());
    std::vector<unsigned char> occupied(vertices.size(), 0);

    float minX = xs.front();
    float maxX = xs.back();
    float minZ = zs.front();
    float maxZ = zs.back();
    float invWidth = maxX > minX ? 1.0f / (maxX - minX) : 1.0f;
    float invDepth = maxZ > minZ ? 1.0f / (maxZ - minZ) : 1.0f;

    for (const Particle* p : paperParticles) {
        size_t ix = nearestCoordinateIndex(xs, p->position.x);
        size_t iz = nearestCoordinateIndex(zs, p->position.z);
        size_t idx = iz * xs.size() + ix;

        vertices[idx] = {
            glm::vec3(p->position),
            p->color,
            p->velocity.w,
            glm::vec2((p->position.x - minX) * invWidth, (p->position.z - minZ) * invDepth)
        };
        occupied[idx] = 1;
    }

    std::vector<uint32_t> indices;
    indices.reserve((xs.size() - 1) * (zs.size() - 1) * 6);

    for (size_t z = 0; z + 1 < zs.size(); ++z) {
        for (size_t x = 0; x + 1 < xs.size(); ++x) {
            uint32_t i0 = static_cast<uint32_t>(z * xs.size() + x);
            uint32_t i1 = static_cast<uint32_t>(z * xs.size() + x + 1);
            uint32_t i2 = static_cast<uint32_t>((z + 1) * xs.size() + x);
            uint32_t i3 = static_cast<uint32_t>((z + 1) * xs.size() + x + 1);

            if (!occupied[i0] || !occupied[i1] || !occupied[i2] || !occupied[i3]) {
                continue;
            }

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    if (indices.empty()) {
        return;
    }

    glGenVertexArrays(1, &paperMeshVAO);
    glGenBuffers(1, &paperMeshVBO);
    glGenBuffers(1, &paperMeshEBO);

    glBindVertexArray(paperMeshVAO);

    glBindBuffer(GL_ARRAY_BUFFER, paperMeshVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(PaperMeshVertex),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, paperMeshEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(uint32_t),
        indices.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(PaperMeshVertex),
        reinterpret_cast<void*>(offsetof(PaperMeshVertex, position))
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(PaperMeshVertex),
        reinterpret_cast<void*>(offsetof(PaperMeshVertex, color))
    );

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(PaperMeshVertex),
        reinterpret_cast<void*>(offsetof(PaperMeshVertex, burnStart))
    );

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(PaperMeshVertex),
        reinterpret_cast<void*>(offsetof(PaperMeshVertex, uv))
    );

    glBindVertexArray(0);

    paperMeshIndexCount = static_cast<uint32_t>(indices.size());
    std::cout << "[ParticleSystem] Paper mesh created: " << vertices.size()
              << " vertices, " << paperMeshIndexCount / 3 << " triangles.\n";
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
        std::cerr << "Failed to load particle render shaders.\n";
    }

    if (!paperMeshShader.loadGraphics(shaderDir + "/paper_mesh.vert",
                                      shaderDir + "/paper_mesh.frag")) {
        std::cerr << "Failed to load paper mesh shaders.\n";
    }

    if (!updateShader.loadCompute(shaderDir + "/particle_update.comp")) {
        std::cerr << "Failed to load particle_update.comp\n";
    }

    if (!emitShader.loadCompute(shaderDir + "/particle_emit.comp")) {
        std::cerr << "Failed to load particle_emit.comp\n";
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

    shader.setFloat("deltaTime", deltaTime);
    shader.setFloat("currentTime", currentTime);
    shader.setVec3("gravity", gravity);
    shader.setFloat("buoyancyStrength", buoyancyStrength);
    shader.setVec3("windDirection", windDirection);
    shader.setFloat("windStrength", windStrength);

    shader.setFloat("u_deltaTime", deltaTime);
    shader.setFloat("u_time", currentTime);
    shader.setUint("u_maxParticles", maxParticles);
    shader.setVec3("u_gravity", gravity);
    shader.setFloat("u_buoyancy", buoyancyStrength);
    shader.setVec3("u_windDir", windDirection);
    shader.setFloat("u_windStrength", windStrength);

    shader.setFloat("u_curlFrequency", curlParams.frequency);
    shader.setFloat("u_curlAmplitude", curlParams.amplitude);
    shader.setInt("u_curlOctaves", curlParams.octaves);
    shader.setFloat("u_curlLacunarity", curlParams.lacunarity);
    shader.setFloat("u_curlPersistence", curlParams.persistence);
    shader.setFloat("u_curlTimeScale", curlParams.timeScale);
    shader.setFloat("u_curlEpsilon", curlParams.epsilon);

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

    emitShader.setFloat("u_deltaTime", deltaTime);
    emitShader.setFloat("u_time", currentTime);
    emitShader.setUint("u_maxParticles", maxParticles);
    emitShader.setUint("u_numEmitters", numEmitters);

    emitShader.setFloat("deltaTime", deltaTime);
    emitShader.setFloat("currentTime", currentTime);
    emitShader.setUint("numEmitters", numEmitters);

    GLuint maxEmitThisFrame = std::max<GLuint>(256, maxParticles / 20);
    emitShader.setUint("maxParticlesToEmitThisFrame", maxEmitThisFrame);

    if (atomicBuffer) {
        GLuint zero = 0;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
        glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicBuffer);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, emitterSSBO);

    GLuint numGroups = (maxParticles + 255) / 256;

    glDispatchCompute(numGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
}

void ParticleSystem::update(float deltaTime, float currentTime) {
    lastRenderTime = currentTime;
    dispatchUpdateCompute(deltaTime, currentTime);
    dispatchEmitCompute(deltaTime, currentTime);
}

void ParticleSystem::renderPaperMesh(const Camera& camera, float aspectRatio, float currentTime) {
    if (paperMeshIndexCount == 0 || paperMeshVAO == 0) {
        return;
    }

    paperMeshShader.use();
    paperMeshShader.setMat4("u_view", camera.getViewMatrix());
    paperMeshShader.setMat4("u_projection", camera.getProjectionMatrix(aspectRatio));
    paperMeshShader.setFloat("u_time", currentTime);
    paperMeshShader.setFloat("u_burnDuration", 4.8f);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glBindVertexArray(paperMeshVAO);
    glDrawElements(GL_TRIANGLES, paperMeshIndexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void ParticleSystem::render(const Camera& camera, float aspectRatio) {
    if (maxParticles == 0) {
        return;
    }

    renderPaperMesh(camera, aspectRatio, lastRenderTime);

    renderShader.use();

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(aspectRatio);
    glm::vec3 camRight = camera.getRight();
    glm::vec3 camUp = camera.getUp();

    renderShader.setMat4("view", view);
    renderShader.setMat4("projection", proj);
    renderShader.setVec3("cameraRight", camRight);
    renderShader.setVec3("cameraUp", camUp);

    renderShader.setMat4("u_view", view);
    renderShader.setMat4("u_projection", proj);
    renderShader.setVec3("u_cameraRight", camRight);
    renderShader.setVec3("u_cameraUp", camUp);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
    glBindVertexArray(particleVAO);

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_POINTS, 0, maxParticles);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindVertexArray(0);
}
