#pragma once
// =============================================================================
// ParticleSystem.h - Sistema de partículas acelerado por GPU
// Proyecto: Simulación de Fuego con Curl Noise
// =============================================================================

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

    // Inicializar con partículas pre-generadas (material + pool de fuego muerto)
    void init(const std::vector<Particle>& initialParticles,
              const std::vector<EmitterConfig>& emitters,
              const std::string& shaderDir = "shaders");

    // Actualizar partículas via compute shaders
    void update(float deltaTime, float currentTime);

    // Renderizar partículas como billboards
    void render(const Camera& camera, float aspectRatio);

    // Configurar parámetros de curl noise
    void setCurlNoiseParams(const CurlNoiseParams& params);

    // Configurar fuerzas globales
    void setGravity(const glm::vec3& g) { gravity = g; }
    void setBuoyancy(float b) { buoyancyStrength = b; }
    void setWind(const glm::vec3& dir, float strength);

    // Escala global de emisión [0,1] (para que el fuego suba y se apague)
    void setEmitScale(float s) { emitScale = s; }

    // Información
    uint32_t getMaxParticles() const { return maxParticles; }
    uint32_t getActiveParticles() const { return activeParticles; }

    // Cleanup
    void cleanup();

private:
    // --- GPU Resources ---
    GLuint particleSSBO = 0;    // Buffer de partículas
    GLuint emitterSSBO  = 0;    // Buffer de emisores
    GLuint particleVAO  = 0;    // VAO para renderizado
    GLuint atomicBuffer = 0;    // Atomic counter para emisión

    // --- Shaders ---
    Shader updateShader;        // particle_update.comp
    Shader emitShader;          // particle_emit.comp
    Shader renderShader;        // particle.vert + .geom + .frag

    // --- Parámetros ---
    uint32_t maxParticles = 0;
    uint32_t activeParticles = 0;
    uint32_t numEmitters = 0;

    CurlNoiseParams curlParams;
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    float buoyancyStrength = 3.0f;
    glm::vec3 windDirection = glm::vec3(0.0f);
    float windStrength = 0.0f;
    float emitScale = 1.0f;

    // --- Métodos internos ---
    void createSSBO(const std::vector<Particle>& particles);
    void createEmitterSSBO(const std::vector<EmitterConfig>& emitters);
    void loadShaders(const std::string& shaderDir);
    void setupVAO();
    void dispatchUpdateCompute(float deltaTime, float currentTime);
    void dispatchEmitCompute(float deltaTime, float currentTime);
    void setComputeUniforms(Shader& shader, float deltaTime, float currentTime);
};
