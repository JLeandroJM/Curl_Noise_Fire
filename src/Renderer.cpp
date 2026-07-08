#include "Renderer.h"
#include <iostream>

Renderer::Renderer() {}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::init(int w, int h, const std::string& shaderDir) {
    width = w;
    height = h;

    createHDRFramebuffer();
    postProcessor.init(width, height, shaderDir);
    setupFullscreenQuad();

    if (!backgroundShader.loadGraphics(shaderDir + "/background.vert", shaderDir + "/background.frag")) {
        std::cerr << "Fallo al cargar shaders de fondo.\n";
    }
}

void Renderer::cleanup() {
    if (hdrFBO) glDeleteFramebuffers(1, &hdrFBO);
    if (hdrColorTexture) glDeleteTextures(1, &hdrColorTexture);
    if (hdrDepthRBO) glDeleteRenderbuffers(1, &hdrDepthRBO);
    if (fullscreenVAO) glDeleteVertexArrays(1, &fullscreenVAO);

    hdrFBO = 0;
    hdrColorTexture = 0;
    hdrDepthRBO = 0;
    fullscreenVAO = 0;
    
    postProcessor.cleanup();
}

void Renderer::createHDRFramebuffer() {
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    // Color attachment (floating-point for HDR)
    glGenTextures(1, &hdrColorTexture);
    glBindTexture(GL_TEXTURE_2D, hdrColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorTexture, 0);

    // Depth attachment
    glGenRenderbuffers(1, &hdrDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, hdrDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: HDR Framebuffer no está completo.\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::setupFullscreenQuad() {
    glGenVertexArrays(1, &fullscreenVAO);
    // VBO is empty, drawn using gl_VertexID in vertex shader (3 vertices for fullscreen triangle)
}

void Renderer::resize(int w, int h) {
    if (w <= 0 || h <= 0) return;
    width = w;
    height = h;

    glBindTexture(GL_TEXTURE_2D, hdrColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

    glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

    postProcessor.resize(width, height);
}

void Renderer::renderBackground(const glm::vec3& color) {
    backgroundShader.use();
    backgroundShader.setVec3("u_bgColor", color);

    glBindVertexArray(fullscreenVAO);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

// void Renderer::renderFrame(ParticleSystem& particleSystem, const Camera& camera, const glm::vec3& backgroundColor) {
//     // 1. Render to HDR Framebuffer
//     glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
//     glViewport(0, 0, width, height);
//     glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//     renderBackground(backgroundColor);

//     float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
//     particleSystem.render(camera, aspectRatio);

//     glBindFramebuffer(GL_FRAMEBUFFER, 0);

//     // 2. Post-processing (Bloom + Tone Mapping)
//     glViewport(0, 0, width, height);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
//     postProcessor.process(hdrColorTexture, bloomThreshold, bloomIterations, bloomIntensity);
// }

void Renderer::renderFrame(ParticleSystem& particleSystem, const Camera& camera, const glm::vec3& backgroundColor) {
    // DEBUG DIRECTO:
    // Renderizamos directamente al framebuffer principal, sin HDR ni bloom.
    // Esto sirve para validar si el problema está en PostProcessor.

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    // Fondo visible para no confundir con negro absoluto.
    glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    // Renderizar partículas directamente.
    particleSystem.render(camera, aspectRatio);
}


void Renderer::readPixels(std::vector<uint8_t>& buffer) const {
    buffer.resize(width * height * 3);
    // Leer los pixeles del framebuffer default
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
}
