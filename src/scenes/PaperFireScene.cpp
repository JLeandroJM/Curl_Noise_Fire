#include "Scene.h"
#include <cmath>
#include <iostream>
#include "PaperFireScene.h"

class PaperFireScene : public Scene {
public:
    std::string getName() const override { return "PaperFire"; }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight = false) override {

        // ¡ELIMINADO: particles.clear();!
        // Ahora conservamos el pool de partículas muertas que inyectó SceneManager.
        emitters.clear();

        // Material colors
        glm::vec4 paperColor(0.96f, 0.93f, 0.84f, 1.0f);
        glm::vec4 woodColor(0.30f, 0.18f, 0.10f, 1.0f);

        // Particle counts based on lightweight mode
        int deskCountU = lightweight ? 80 : 150;
        int deskCountV = lightweight ? 60 : 100;
        int paperCountU = lightweight ? 120 : 250;
        int paperCountV = lightweight ? 90 : 180;

        float pSize = lightweight ? 0.026f : 0.014f;

        // Desk
        glm::vec3 deskCenter(0.0f, 0.0f, 0.0f);
        GeometryUtils::generatePlane(particles, deskCenter,
                                     glm::vec3(1,0,0), 3.0f, deskCountU,
                                     glm::vec3(0,0,1), 2.0f, deskCountV,
                                     TYPE_WOOD, woodColor, pSize);

        // Paper (slightly above desk)
        glm::vec3 paperCenter(0.0f, 0.01f, 0.0f);
        GeometryUtils::generatePlane(particles, paperCenter,
                                     glm::vec3(1,0,0), 1.0f, paperCountU,
                                     glm::vec3(0,0,1), 0.75f, paperCountV,
                                     TYPE_PAPER, paperColor, pSize);

        // Ignition point: corner of the paper
        std::vector<glm::vec3> ignitionPoints = { paperCenter - glm::vec3(0.5f, 0.0f, 0.375f) };
        float burnSpeed = 0.18f;
        float randomVariation = 0.45f;
        GeometryUtils::computeBurnTimes(particles, ignitionPoints, burnSpeed, randomVariation);

        // ¡ELIMINADO: El bucle for que creaba las 50,000 partículas muertas!

        // Curl noise params for small flames
        curlParams.frequency = 3.0f;
        curlParams.amplitude = 0.5f;
        curlParams.octaves = 3;
        curlParams.timeScale = 1.5f;
        curlParams.boundaryWidth = 0.1f;

        // El fuego visible sale del frente de papel en combustion, no de un
        // emisor fijo en la esquina.

        // Camera path
        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        cameraPath.keyframes.push_back({0.0f, glm::vec3(-1.5f, 1.0f, 1.5f), deskCenter});
        cameraPath.keyframes.push_back({7.5f, glm::vec3(0.0f, 1.5f, 2.0f), deskCenter});
        cameraPath.keyframes.push_back({15.0f, glm::vec3(1.5f, 1.0f, 1.5f), deskCenter});
    }

    float getDuration() const override { return 15.0f; }
};

std::unique_ptr<Scene> createPaperFireScene() {
    return std::make_unique<PaperFireScene>();
}
