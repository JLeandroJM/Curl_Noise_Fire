#pragma once
// =============================================================================
// Renderer.h - Renderizador principal con pipeline HDR + Bloom
// Proyecto: Simulación de Fuego con Curl Noise
// =============================================================================

#include <glad/gl.h>
#include <glm/glm.hpp>
#include "ParticleSystem.h"
#include "Camera.h"
#include "PostProcessor.h"
#include "Shader.h"
#include <string>

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Inicializar con resolución
    void init(int width, int height, const std::string& shaderDir = "shaders");

    // Renderizar un frame completo
    void renderFrame(ParticleSystem& particleSystem, const Camera& camera,
                     const glm::vec3& backgroundColor);

    // Resize
    void resize(int width, int height);

    // Configuración de bloom
    void setBloomIntensity(float intensity) { bloomIntensity = intensity; }
    void setBloomThreshold(float threshold) { bloomThreshold = threshold; }
    void setBloomIterations(int iters) { bloomIterations = iters; }

    // Leer píxeles del framebuffer (para exportar frames)
    void readPixels(std::vector<uint8_t>& buffer) const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    void cleanup();

private:
    int width = 1920;
    int height = 1080;

    // HDR Framebuffer
    GLuint hdrFBO = 0;
    GLuint hdrColorTexture = 0;
    GLuint hdrDepthRBO = 0;

    // Post-processing
    PostProcessor postProcessor;

    // Background rendering
    Shader backgroundShader;
    GLuint fullscreenVAO = 0;

    // Configuración bloom
    float bloomIntensity = 1.2f;
    float bloomThreshold = 0.8f;
    int bloomIterations = 5;

    void createHDRFramebuffer();
    void renderBackground(const glm::vec3& color);
    void setupFullscreenQuad();
};
