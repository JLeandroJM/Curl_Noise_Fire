#include "Scene.h"
#include <cmath>

class PaperFireScene : public Scene {
public:
    std::string getName() const override { return "PaperFire"; }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight = false) override {
        
        particles.clear();
        emitters.clear();

        // Material colors
        glm::vec4 paperColor(0.95f, 0.9f, 0.8f, 1.0f);
        glm::vec4 woodColor(0.55f, 0.35f, 0.15f, 1.0f);

        // Particle counts based on lightweight mode
        int deskCountU = lightweight ? 40 : 100;
        int deskCountV = lightweight ? 30 : 60;
        int paperCountU = lightweight ? 30 : 80;
        int paperCountV = lightweight ? 20 : 60;
        size_t deadPoolSize = lightweight ? 20000 : 50000;

        float pSize = lightweight ? 0.04f : 0.02f;

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
        float burnSpeed = 0.3f;
        float randomVariation = 0.1f;
        GeometryUtils::computeBurnTimes(particles, ignitionPoints, burnSpeed, randomVariation);

        // Add dead particles for fire
        for (size_t i = 0; i < deadPoolSize; ++i) {
            Particle p{};
            p.type = TYPE_DEAD;
            particles.push_back(p);
        }

        // Curl noise params for small flames
        curlParams.frequency = 3.0f;
        curlParams.amplitude = 0.5f;
        curlParams.octaves = 3;
        curlParams.timeScale = 1.5f;
        curlParams.boundaryWidth = 0.1f;

        // No continuous emitters needed, the material will burn, 
        // but let's add one tiny emitter at ignition just to spark it
        EmitterConfig spark;
        spark.position = ignitionPoints[0];
        spark.shape = EmitterShape::POINT;
        spark.emitRate = 100.0f;
        spark.particleLife = 1.0f;
        spark.initialSpeed = 0.5f;
        spark.particleSize = 0.03f;
        emitters.push_back(spark);

        // Camera path
        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        cameraPath.keyframes.push_back({0.0f, glm::vec3(-1.5f, 1.0f, 1.5f), deskCenter});
        cameraPath.keyframes.push_back({7.5f, glm::vec3(0.0f, 1.5f, 2.0f), deskCenter});
        cameraPath.keyframes.push_back({15.0f, glm::vec3(1.5f, 1.0f, 1.5f), deskCenter});
    }

    float getDuration() const override { return 15.0f; }
};
