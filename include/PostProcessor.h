#pragma once
// =============================================================================
// PostProcessor.h - Utilidad para Post-Processing (Bloom)
// Proyecto: Simulación de Fuego con Curl Noise
// =============================================================================

#include <glad/gl.h>
#include "Shader.h"
#include <string>
#include <vector>

class PostProcessor {
public:
    PostProcessor();
    ~PostProcessor();

    // Inicializar con resolución
    void init(int width, int height, const std::string& shaderDir = "shaders");

    // Redimensionar buffers
    void resize(int width, int height);

    // Procesar: toma una textura HDR y dibuja a la pantalla (o FBO actual)
    void process(GLuint hdrColorTexture, float threshold, int iterations, float intensity);

    void cleanup();

private:
    int width = 0;
    int height = 0;

    // Ping-pong FBOs para blur en dos pasadas
    GLuint pingpongFBO[2] = {0, 0};
    GLuint pingpongColorBuffers[2] = {0, 0};

    // Shaders
    Shader bloomExtractShader;
    Shader blurShader;
    Shader bloomCombineShader;

    // Quad de renderizado
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    void setupRenderQuad();
    void createBuffers();
};
