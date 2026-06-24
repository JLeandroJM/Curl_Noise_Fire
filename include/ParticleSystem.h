#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include "Particle.h"
#include "Shader.h"
#include "Camera.h"
#include <vector>
#include <string>

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    void init(const std::vector<Particle>& initialParticles,
              const std::vector<EmitterConfig>& emitters,
              const std::string& shaderDir = "shaders");

    void update(float deltaTime, float currentTime);
    void render(const Camera& camera, float aspectRatio);

    void setCurlNoiseParams(const CurlNoiseParams& params);

    void setGravity(const glm::vec3& g) { gravity = g; }
    void setBuoyancy(float b) { buoyancyStrength = b; }
    void setWind(const glm::vec3& dir, float strength);

    uint32_t getMaxParticles() const { return maxParticles; }
    uint32_t getActiveParticles() const { return activeParticles; }

    void cleanup();

private:
    GLuint particleSSBO = 0;
    GLuint emitterSSBO = 0;
    GLuint particleVAO = 0;
    GLuint atomicBuffer = 0;

    Shader updateShader;
    Shader emitShader;
    Shader renderShader;

    uint32_t maxParticles = 0;
    uint32_t activeParticles = 0;
    uint32_t numEmitters = 0;

    CurlNoiseParams curlParams;
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    float buoyancyStrength = 3.0f;
    glm::vec3 windDirection = glm::vec3(0.0f);
    float windStrength = 0.0f;

    void createSSBO(const std::vector<Particle>& particles);
    void createEmitterSSBO(const std::vector<EmitterConfig>& emitters);
    void loadShaders(const std::string& shaderDir);
    void setupVAO();
    void dispatchUpdateCompute(float deltaTime, float currentTime);
    void dispatchEmitCompute(float deltaTime, float currentTime);
    void setComputeUniforms(Shader& shader, float deltaTime, float currentTime);
};
