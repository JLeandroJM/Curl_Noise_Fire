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

        // Las llamas nacen de las particulas de papel que estan en fase de
        // combustion, para que el frente avance con el deterioro de la hoja.
        
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
                glm::vec2 fromIgnition(pos.x - ignitionPoints[0].x, pos.z - ignitionPoints[0].z);
                float diagonalPush = glm::dot(
                    glm::normalize(fromIgnition + glm::vec2(0.001f)),
                    glm::normalize(glm::vec2(1.0f, 0.72f))
                );

                float tendrils = sin(pos.x * 18.0f + pos.z * 10.0f)
                               + 0.55f * sin(pos.x * 37.0f - pos.z * 24.0f + 1.7f);
                float dampPatches = sin(pos.x * 7.0f - 0.8f) * cos(pos.z * 6.5f + 0.4f);
                float noise = tendrils * 0.42f + dampPatches * 0.70f + dist(rng) * 0.22f;
                              
                float thermalAcceleration = std::pow(minDist, 0.88f);
                float fragility = (dist(rng) > 0.92f) ? 0.35f : 1.0f;
                
                // burnTime variará entre 0 y ~12 segundos a lo largo de la hoja
                float burnTime = (thermalAcceleration / baseBurnSpeed) + noise * 1.15f * fragility;
                burnTime -= glm::clamp(diagonalPush, 0.0f, 1.0f) * 0.35f;
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
