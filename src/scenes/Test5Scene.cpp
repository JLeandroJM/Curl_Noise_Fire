#include "Test5Scene.h"
#include <iostream>
#include <random>

class Test5SceneImpl : public Scene {
public:
    std::string getName() const override {
        return "Escena de prueba test 5";
    }

    float getDuration() const override {
        return 28.0f;
    }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(1.0f, 1.0f, 1.0f); // Fondo totalmente blanco
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {
        
        emitters.clear();

        // Papel claro antes de quemarse.
        glm::vec4 paperColor(0.97f, 0.94f, 0.86f, 1.0f);

        // Papel: Densidad extrema para formar una superficie sólida
        int paperU = lightweight ? 350 : 500;
        int paperV = lightweight ? 250 : 350;

        float paperParticleSize = lightweight ? 0.0048f : 0.0042f;

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
        float baseBurnSpeed = 0.085f;

        EmitterConfig flameFront;
        flameFront.position = ignitionPoints[0] + glm::vec3(0.04f, 0.018f, 0.04f);
        flameFront.shape = EmitterShape::DISK;
        flameFront.radius = 0.055f;
        flameFront.direction = glm::vec3(0.14f, 1.0f, 0.08f);
        flameFront.emitRate = lightweight ? 1800.0f : 3400.0f;
        flameFront.particleLife = 0.85f;
        flameFront.lifeVariance = 0.30f;
        flameFront.initialSpeed = 0.72f;
        flameFront.speedVariance = 0.36f;
        flameFront.temperature = 1.0f;
        flameFront.particleSize = lightweight ? 0.020f : 0.016f;
        emitters.push_back(flameFront);
        
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
                float noise = sin(pos.x * 10.0f) * cos(pos.z * 9.0f) * 0.85f +
                              sin(pos.x * 23.0f + 1.2f) * cos(pos.z * 21.0f + 0.5f) * 0.45f +
                              dist(rng) * 0.35f;
                              
                float thermalAcceleration = std::pow(minDist, 0.90f);
                float fragility = (dist(rng) > 0.92f) ? 0.35f : 1.0f;
                
                // burnTime variará entre 0 y ~12 segundos a lo largo de la hoja
                float burnTime = (thermalAcceleration / baseBurnSpeed) + noise * 1.5f * fragility;
                p.velocity.w = std::max(0.0f, burnTime);
            }
        }

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
        cameraPath.keyframes.push_back({28.0f, camPos, target});
    }
};

std::unique_ptr<Scene> createTest5Scene() {
    return std::make_unique<Test5SceneImpl>();
}
