#pragma once
// =============================================================================
// Scene.h - Interfaz base para todas las escenas
// Proyecto: Simulación de Fuego con Curl Noise
// =============================================================================

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Particle.h"
#include "Camera.h"

// Funciones útiles para generar geometría de partículas
namespace GeometryUtils {
    // Rellena un plano con partículas
    void generatePlane(std::vector<Particle>& particles,
                       const glm::vec3& center,
                       const glm::vec3& u_dir, float u_size, int u_count,
                       const glm::vec3& v_dir, float v_size, int v_count,
                       uint32_t type, const glm::vec4& color, float pSize);

    // Crea un cilindro de partículas (e.g. tronco de árbol)
    void generateCylinder(std::vector<Particle>& particles,
                          const glm::vec3& base, const glm::vec3& dir,
                          float radius, float height, int radial_count, int height_count,
                          uint32_t type, const glm::vec4& color, float pSize);

    // Crea una esfera de partículas (e.g. hojas de árbol)
    void generateSphere(std::vector<Particle>& particles,
                        const glm::vec3& center, float radius, int count,
                        uint32_t type, const glm::vec4& color, float pSize);

    // Calcula burnStartTime para las partículas basado en distancia a puntos de ignición
    void computeBurnTimes(std::vector<Particle>& particles,
                          const std::vector<glm::vec3>& ignitionPoints,
                          float burnSpeed, float randomVariation);

    // Crea una caja de partículas
    void generateBox(std::vector<Particle>& particles,
                     const glm::vec3& center, const glm::vec3& size,
                     int countX, int countY, int countZ,
                     uint32_t type, const glm::vec4& color, float pSize);

    // Crea una línea de partículas
    void generateLine(std::vector<Particle>& particles,
                      const glm::vec3& start, const glm::vec3& end, int count,
                      uint32_t type, const glm::vec4& color, float pSize);
}

class Scene {
public:
    virtual ~Scene() = default;

    virtual std::string getName() const = 0;

    // Configura la escena: rellena partículas, emisores, curlParams y la ruta de la cámara
    virtual void setup(std::vector<Particle>& particles,
                       std::vector<EmitterConfig>& emitters,
                       CurlNoiseParams& curlParams,
                       CameraPath& cameraPath,
                       bool lightweight = false) = 0; // lightweight para GTX 1650

    // Duración de la escena en segundos
    virtual float getDuration() const = 0;

    // Color de fondo
    virtual glm::vec3 getBackgroundColor() const { return glm::vec3(0.02f); }
};
