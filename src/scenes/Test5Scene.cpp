#include "Test5Scene.h"
#include <iostream>
#include <random>

class Test5SceneImpl : public Scene {
public:
    std::string getName() const override {
        return "Escena de prueba test 5";
    }

    float getDuration() const override {
        return 20.0f;
    }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(1.0f, 1.0f, 1.0f); // Fondo totalmente blanco
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {
        
        particles.clear();
        emitters.clear();

        // Color ROJO para el papel
        glm::vec4 paperColor(0.9f, 0.1f, 0.1f, 1.0f); 

        // Papel: Densidad extrema para formar una superficie sólida
        int paperU = lightweight ? 350 : 500;
        int paperV = lightweight ? 250 : 350;

        float paperParticleSize = lightweight ? 0.005f : 0.004f;

        // Quitamos la mesa. Solo generamos el papel.
        glm::vec3 paperCenter(0.0f, 0.0f, 0.0f);
        GeometryUtils::generatePlane(
            particles,
            paperCenter,
            glm::vec3(1.0f, 0.0f, 0.0f), 1.35f, paperU,
            glm::vec3(0.0f, 0.0f, 1.0f), 0.90f, paperV,
            TYPE_PAPER,
            paperColor,
            paperParticleSize
        );

        // PUNTO DE IGNICIÓN (Esquina del papel)
        std::vector<glm::vec3> ignitionPoints = { paperCenter - glm::vec3(0.6f, 0.0f, 0.4f) };
        float baseBurnSpeed = 0.15f; // Velocidad MUCHO más lenta (antes 0.45f)
        
        // PROPAGACIÓN REALISTA (Simulación de humedad/espesor)
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        for (auto& p : particles) {
            if (p.type == TYPE_PAPER) {
                float minDist = 1e9f;
                glm::vec3 pos = glm::vec3(p.position);
                for (const auto& ip : ignitionPoints) {
                    minDist = std::min(minDist, glm::distance(pos, ip));
                }
                
                // Ruido más agresivo para crear islas que tardan mucho en quemarse (zonas húmedas/densas)
                float noise = sin(pos.x * 12.0f) * cos(pos.z * 12.0f) * 1.5f +
                              sin(pos.x * 25.0f + 1.2f) * cos(pos.z * 25.0f + 0.5f) * 0.8f +
                              dist(rng) * 0.3f;
                              
                float thermalAcceleration = std::pow(minDist, 0.90f);
                float fragility = (dist(rng) > 0.85f) ? 0.2f : 1.0f; // Agujeros que se forman casi al instante
                
                // burnTime variará entre 0 y ~12 segundos a lo largo de la hoja
                float burnTime = (thermalAcceleration / baseBurnSpeed) + noise * 1.5f * fragility;
                p.velocity.w = std::max(0.0f, burnTime);
            }
        }

        // Emisor de "chispa" inicial solo para encender el fuego
        EmitterConfig spark;
        spark.position = ignitionPoints[0];
        spark.shape = EmitterShape::POINT;
        spark.emitRate = 100.0f;
        spark.particleLife = 1.0f;
        spark.initialSpeed = 0.5f;
        spark.particleSize = 0.03f;
        emitters.push_back(spark);

        // Configuración de Curl Noise para las llamas
        curlParams.frequency = 3.0f;
        curlParams.amplitude = 1.0f; 
        curlParams.octaves = 3;
        curlParams.lacunarity = 2.0f;
        curlParams.persistence = 0.5f;
        curlParams.timeScale = 1.5f;
        curlParams.epsilon = 0.01f;

        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        glm::vec3 camPos(0.0f, 1.05f, 1.55f);
        glm::vec3 target(0.0f, 0.0f, 0.0f);
        cameraPath.keyframes.push_back({0.0f, camPos, target});
        cameraPath.keyframes.push_back({20.0f, camPos, target});
    }
};

std::unique_ptr<Scene> createTest5Scene() {
    return std::make_unique<Test5SceneImpl>();
}
