#include "Scene.h"

class WallFireScene : public Scene {
public:
    std::string getName() const override { return "WallFire"; }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight = false) override {
        
        particles.clear();
        emitters.clear();

        glm::vec4 cementColor(0.6f, 0.58f, 0.55f, 1.0f);

        int wallCountU = lightweight ? 40 : 120;
        int wallCountV = lightweight ? 30 : 90;
        size_t deadPoolSize = lightweight ? 30000 : 100000;
        float pSize = lightweight ? 0.08f : 0.04f;

        glm::vec3 wallCenter(0.0f, 2.0f, 0.0f); // Center of wall
        
        // Wall
        GeometryUtils::generatePlane(particles, wallCenter,
                                     glm::vec3(1,0,0), 4.0f, wallCountU,
                                     glm::vec3(0,1,0), 4.0f, wallCountV,
                                     TYPE_CEMENT, cementColor, pSize);

        // Ground in front of wall
        GeometryUtils::generatePlane(particles, glm::vec3(0.0f, 0.0f, 1.0f),
                                     glm::vec3(1,0,0), 4.0f, wallCountU,
                                     glm::vec3(0,0,1), 2.0f, wallCountV/2,
                                     TYPE_GROUND, glm::vec4(0.35f, 0.25f, 0.15f, 1.0f), pSize);

        // Cement doesn't really burn, so burnSpeed is practically zero or very slow, 
        // but we can set it so it doesn't burn, just emits.
        std::vector<glm::vec3> ignitionPoints = { glm::vec3(0.0f, 0.0f, 0.1f) };
        GeometryUtils::computeBurnTimes(particles, ignitionPoints, 0.01f, 0.1f);

        // Add dead particles for fire
        for (size_t i = 0; i < deadPoolSize; ++i) {
            Particle p{};
            p.type = TYPE_DEAD;
            particles.push_back(p);
        }

        // Curl noise params for medium turbulence
        curlParams.frequency = 1.0f;
        curlParams.amplitude = 1.5f;
        curlParams.octaves = 4;
        curlParams.timeScale = 1.0f;
        curlParams.boundaryWidth = 0.5f;

        // Emitters along the wall base
        EmitterConfig baseEmitter;
        baseEmitter.position = glm::vec3(0.0f, 0.1f, 0.1f);
        baseEmitter.direction = glm::vec3(0.0f, 1.0f, 0.05f);
        baseEmitter.shape = EmitterShape::LINE;
        baseEmitter.width = 3.0f; // length of the line
        baseEmitter.emitRate = lightweight ? 5000.0f : 15000.0f;
        baseEmitter.particleLife = 2.5f;
        baseEmitter.lifeVariance = 0.5f;
        baseEmitter.initialSpeed = 2.0f;
        baseEmitter.speedVariance = 1.0f;
        baseEmitter.particleSize = 0.05f;
        emitters.push_back(baseEmitter);

        // Camera path: pull-back
        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        cameraPath.keyframes.push_back({0.0f, glm::vec3(0.0f, 1.0f, 1.5f), glm::vec3(0.0f, 2.0f, 0.0f)});
        cameraPath.keyframes.push_back({10.0f, glm::vec3(0.0f, 1.5f, 3.5f), glm::vec3(0.0f, 2.0f, 0.0f)});
        cameraPath.keyframes.push_back({20.0f, glm::vec3(0.0f, 2.0f, 6.0f), glm::vec3(0.0f, 2.0f, 0.0f)});
    }

    float getDuration() const override { return 20.0f; }
};
