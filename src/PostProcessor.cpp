#include "PostProcessor.h"
#include <iostream>

PostProcessor::PostProcessor() {}

PostProcessor::~PostProcessor() {
    cleanup();
}

void PostProcessor::cleanup() {
    for (int i = 0; i < 2; i++) {
        if (pingpongFBO[i]) glDeleteFramebuffers(1, &pingpongFBO[i]);
        if (pingpongColorBuffers[i]) glDeleteTextures(1, &pingpongColorBuffers[i]);
        pingpongFBO[i] = 0;
        pingpongColorBuffers[i] = 0;
    }
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    quadVAO = 0;
}

void PostProcessor::init(int w, int h, const std::string& shaderDir) {
    width = w;
    height = h;

    if (!bloomExtractShader.loadGraphics(shaderDir + "/post.vert", shaderDir + "/bloom_extract.frag")) {
        std::cerr << "Fallo al cargar bloom_extract.frag\n";
    }
    if (!blurShader.loadGraphics(shaderDir + "/post.vert", shaderDir + "/blur.frag")) {
        std::cerr << "Fallo al cargar blur.frag\n";
    }
    if (!bloomCombineShader.loadGraphics(shaderDir + "/post.vert", shaderDir + "/bloom_combine.frag")) {
        std::cerr << "Fallo al cargar bloom_combine.frag\n";
    }

    createBuffers();
    setupRenderQuad();
}

void PostProcessor::createBuffers() {
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorBuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[i]);
        // Fila flotante para blur
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorBuffers[i], 0);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "PingPong FBO not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::resize(int w, int h) {
    if (w <= 0 || h <= 0) return;
    width = w;
    height = h;

    for (unsigned int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    }
}

void PostProcessor::setupRenderQuad() {
    glGenVertexArrays(1, &quadVAO);
    // drawn using gl_VertexID (3 vertices for fullscreen triangle)
}

void PostProcessor::process(GLuint hdrColorTexture, float threshold, int iterations, float intensity) {
    glBindVertexArray(quadVAO);
    glDisable(GL_DEPTH_TEST);

    // 1. Extraer brillo
    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[0]);
    bloomExtractShader.use();
    bloomExtractShader.setFloat("u_threshold", threshold);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrColorTexture);
    bloomExtractShader.setInt("u_image", 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // 2. Blur (ping-pong)
    bool horizontal = true;
    bool first_iteration = true;
    int amount = iterations * 2;
    blurShader.use();
    for (unsigned int i = 0; i < amount; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        blurShader.setInt("u_horizontal", horizontal);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? pingpongColorBuffers[0] : pingpongColorBuffers[!horizontal]);
        blurShader.setInt("u_image", 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }

    // 3. Combinar y dibujar al framebuffer default (0)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    bloomCombineShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrColorTexture);
    bloomCombineShader.setInt("u_scene", 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[!horizontal]);
    bloomCombineShader.setInt("u_bloomBlur", 1);
    
    bloomCombineShader.setFloat("u_bloomIntensity", intensity);
    bloomCombineShader.setFloat("u_exposure", 1.0f); // Could make it a param
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}
