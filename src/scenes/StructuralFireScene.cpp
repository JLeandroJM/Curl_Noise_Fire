#include "Scene.h"

class StructuralFireScene : public Scene {
public:
    std::string getName() const override { return "StructuralFire"; }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight = false) override {
        
        particles.clear();
        emitters.clear();

        glm::vec4 cementColor(0.6f, 0.58f, 0.55f, 1.0f);
        glm::vec4 woodColor(0.55f, 0.35f, 0.15f, 1.0f);

        size_t deadPoolSize = lightweight ? 50000 : 200000;
        float pSize = lightweight ? 0.08f : 0.04f;

        int colCountX = lightweight ? 5 : 10;
        int colCountY = lightweight ? 30 : 60;
        int colCountZ = lightweight ? 5 : 10;
        
        int wallU = lightweight ? 30 : 80;
        int wallV = lightweight ? 20 : 60;

        // Columns
        GeometryUtils::generateBox(particles, glm::vec3(-2.0f, 1.5f, -2.0f), glm::vec3(0.4f, 3.0f, 0.4f), colCountX, colCountY, colCountZ, TYPE_CEMENT, cementColor, pSize);
        GeometryUtils::generateBox(particles, glm::vec3( 2.0f, 1.5f, -2.0f), glm::vec3(0.4f, 3.0f, 0.4f), colCountX, colCountY, colCountZ, TYPE_CEMENT, cementColor, pSize);
        GeometryUtils::generateBox(particles, glm::vec3(-2.0f, 1.5f,  2.0f), glm::vec3(0.4f, 3.0f, 0.4f), colCountX, colCountY, colCountZ, TYPE_CEMENT, cementColor, pSize);
        GeometryUtils::generateBox(particles, glm::vec3( 2.0f, 1.5f,  2.0f), glm::vec3(0.4f, 3.0f, 0.4f), colCountX, colCountY, colCountZ, TYPE_CEMENT, cementColor, pSize);

        // Beams
        GeometryUtils::generateBox(particles, glm::vec3(0.0f, 3.1f, -2.0f), glm::vec3(4.4f, 0.4f, 0.4f), colCountY, colCountX, colCountZ, TYPE_CEMENT, cementColor, pSize);
        GeometryUtils::generateBox(particles, glm::vec3(0.0f, 3.1f,  2.0f), glm::vec3(4.4f, 0.4f, 0.4f), colCountY, colCountX, colCountZ, TYPE_CEMENT, cementColor, pSize);

        // Interior Wall (back)
        GeometryUtils::generatePlane(particles, glm::vec3(0.0f, 1.5f, -1.8f), 
                                     glm::vec3(1,0,0), 4.0f, wallU,
                                     glm::vec3(0,1,0), 3.0f, wallV,
                                     TYPE_CEMENT, cementColor, pSize);
        
        // Interior Wall (side)
        GeometryUtils::generatePlane(particles, glm::vec3(-1.8f, 1.5f, 0.0f), 
                                     glm::vec3(0,0,1), 4.0f, wallU,
                                     glm::vec3(0,1,0), 3.0f, wallV,
                                     TYPE_CEMENT, cementColor, pSize);

        // Wooden Door Frame
        int doorX = lightweight ? 5 : 10;
        int doorY = lightweight ? 20 : 40;
        GeometryUtils::generateBox(particles, glm::vec3(-0.5f, 1.0f, 1.8f), glm::vec3(0.1f, 2.0f, 0.2f), doorX, doorY, doorX, TYPE_WOOD, woodColor, pSize);
        GeometryUtils::generateBox(particles, glm::vec3( 0.5f, 1.0f, 1.8f), glm::vec3(0.1f, 2.0f, 0.2f), doorX, doorY, doorX, TYPE_WOOD, woodColor, pSize);
        GeometryUtils::generateBox(particles, glm::vec3( 0.0f, 2.0f, 1.8f), glm::vec3(1.1f, 0.1f, 0.2f), doorY, doorX, doorX, TYPE_WOOD, woodColor, pSize);

        // Burn setup - wood burns, cement doesn't
        std::vector<glm::vec3> ignitionPoints = { glm::vec3(0.0f, 0.5f, 0.0f) };
        for (auto& p : particles) {
            if (p.type == TYPE_CEMENT) {
                p.velocity.w = 1000.0f; // Doesn't burn
            } else if (p.type == TYPE_WOOD) {
                float d = glm::distance(glm::vec3(p.position), ignitionPoints[0]);
                p.velocity.w = d / 0.5f + ((rand() % 100) / 1000.0f) + 5.0f; // Wood catches after 5s
            }
        }

        // Add dead particles
        for (size_t i = 0; i < deadPoolSize; ++i) {
            Particle p{};
            p.type = TYPE_DEAD;
            particles.push_back(p);
        }

        // Massive turbulence, low freq
        curlParams.frequency = 0.5f;
        curlParams.amplitude = 3.0f;
        curlParams.octaves = 5;
        curlParams.timeScale = 1.0f;

        // Heavy fire emitting from inside
        EmitterConfig interiorFire;
        interiorFire.position = ignitionPoints[0];
        interiorFire.direction = glm::vec3(0.0f, 1.0f, 0.2f);
        interiorFire.shape = EmitterShape::DISK;
        interiorFire.radius = 1.0f;
        interiorFire.emitRate = lightweight ? 5000.0f : 20000.0f;
        interiorFire.particleLife = 3.0f;
        interiorFire.initialSpeed = 3.0f;
        interiorFire.particleSize = 0.06f;
        emitters.push_back(interiorFire);

        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        cameraPath.keyframes.push_back({0.0f, glm::vec3(0.0f, 1.5f, 8.0f), glm::vec3(0.0f, 1.5f, 0.0f)});
        cameraPath.keyframes.push_back({15.0f, glm::vec3(2.0f, 1.5f, 5.0f), glm::vec3(0.0f, 1.5f, 0.0f)});
        cameraPath.keyframes.push_back({30.0f, glm::vec3(1.0f, 1.5f, 3.0f), glm::vec3(0.0f, 1.5f, 0.0f)});
    }

    float getDuration() const override { return 30.0f; }
};
