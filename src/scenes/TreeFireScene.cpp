#include "Scene.h"
#include <cmath>

class TreeFireScene : public Scene {
public:
    std::string getName() const override { return "TreeFire"; }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight = false) override {
        
        particles.clear();
        emitters.clear();

        glm::vec4 woodColor(0.55f, 0.35f, 0.15f, 1.0f);
        glm::vec4 leafColor(0.2f, 0.65f, 0.15f, 1.0f);
        glm::vec4 groundColor(0.35f, 0.25f, 0.15f, 1.0f);

        int radialCount = lightweight ? 10 : 30;
        int heightCount = lightweight ? 50 : 150;
        int leafCount = lightweight ? 3000 : 15000;
        int groundCount = lightweight ? 30 : 100;
        size_t deadPoolSize = lightweight ? 40000 : 150000;
        float pSize = lightweight ? 0.08f : 0.04f;

        // Ground
        GeometryUtils::generatePlane(particles, glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(1,0,0), 6.0f, groundCount,
                                     glm::vec3(0,0,1), 6.0f, groundCount,
                                     TYPE_GROUND, groundColor, pSize);

        // Trunk
        GeometryUtils::generateCylinder(particles, glm::vec3(0.0f, 0.0f, 0.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f), 0.15f, 3.0f,
                                        radialCount, heightCount,
                                        TYPE_WOOD, woodColor, pSize);

        // Branches
        int branchParticles = lightweight ? 100 : 400;
        std::vector<glm::vec3> branchEnds;
        glm::vec3 trunkTop(0.0f, 3.0f, 0.0f);
        for(int i = 0; i < 6; ++i) {
            float angle = i * 1.047f; // 60 degrees
            float height = 1.5f + (i % 3) * 0.5f;
            glm::vec3 start(0.0f, height, 0.0f);
            glm::vec3 end = start + glm::vec3(std::cos(angle), 0.5f, std::sin(angle)) * 1.5f;
            branchEnds.push_back(end);
            GeometryUtils::generateLine(particles, start, end, branchParticles, TYPE_WOOD, woodColor, pSize);
        }

        // Leaves canopy
        for(const auto& end : branchEnds) {
            GeometryUtils::generateSphere(particles, end, 1.0f, leafCount / 6, TYPE_LEAF, leafColor, pSize);
        }
        GeometryUtils::generateSphere(particles, trunkTop, 1.5f, leafCount / 3, TYPE_LEAF, leafColor, pSize);

        // Burn times
        std::vector<glm::vec3> ignitionPoints = { glm::vec3(0.0f, 0.1f, 0.0f) };
        
        // Custom burn setup because leaves burn faster than wood
        float woodBurnSpeed = 0.4f;
        float leafBurnSpeed = 1.5f;
        for (auto& p : particles) {
            if (p.type == TYPE_GROUND) {
                p.velocity.w = 1000.0f; // Doesn't burn easily
            } else {
                float d = glm::distance(glm::vec3(p.position), ignitionPoints[0]);
                float speed = (p.type == TYPE_LEAF) ? leafBurnSpeed : woodBurnSpeed;
                p.velocity.w = d / speed + ((rand() % 100) / 1000.0f);
            }
        }

        // Add dead particles
        for (size_t i = 0; i < deadPoolSize; ++i) {
            Particle p{};
            p.type = TYPE_DEAD;
            particles.push_back(p);
        }

        curlParams.frequency = 1.5f;
        curlParams.amplitude = 2.0f;
        curlParams.octaves = 4;
        curlParams.timeScale = 1.2f;

        // Spark emitter at base
        EmitterConfig baseEmitter;
        baseEmitter.position = ignitionPoints[0];
        baseEmitter.shape = EmitterShape::POINT;
        baseEmitter.emitRate = 200.0f;
        baseEmitter.particleLife = 1.5f;
        baseEmitter.initialSpeed = 1.0f;
        baseEmitter.particleSize = 0.04f;
        emitters.push_back(baseEmitter);

        // Camera orbit
        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        cameraPath.keyframes.push_back({0.0f,  glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(0.0f, 2.0f, 0.0f)});
        cameraPath.keyframes.push_back({12.5f, glm::vec3(5.0f, 2.0f, 0.0f), glm::vec3(0.0f, 2.0f, 0.0f)});
        cameraPath.keyframes.push_back({25.0f, glm::vec3(0.0f, 3.0f, -5.0f), glm::vec3(0.0f, 2.0f, 0.0f)});
    }

    float getDuration() const override { return 25.0f; }
};
