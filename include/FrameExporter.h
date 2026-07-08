#pragma once
// =============================================================================
// FrameExporter.h - Exporta frames a PNG y opcionalmente a video con OpenCV
// Proyecto: Simulación de Fuego con Curl Noise
// =============================================================================

#include <glad/gl.h>
#include <string>
#include <vector>
#include <memory>

class FrameExporter {
public:
    FrameExporter();
    ~FrameExporter();

    // Iniciar grabación de video (requiere OpenCV)
    bool initVideoWriter(const std::string& filename, int width, int height, double fps = 30.0);

    // Guardar frame a disco
    void addFrame(int width, int height);

    // Finalizar grabación
    void finish();

    // Exportar frame individual como PNG
    bool saveFrameAsPNG(const std::string& filename, int width, int height);

private:
    // Pimpl idiom to hide OpenCV dependency from header
    struct Impl;
    std::unique_ptr<Impl> pimpl;
    
    std::vector<uint8_t> pixelBuffer;
};
