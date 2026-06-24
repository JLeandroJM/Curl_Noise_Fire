#include "ChairBurnScene.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

float hash01(const glm::vec3& p) {
    float v = std::sin(glm::dot(p, glm::vec3(12.9898f, 78.233f, 37.719f))) * 43758.5453f;
    return v - std::floor(v);
}

void assignChairBurnTimes(std::vector<Particle>& particles,
                          const std::vector<glm::vec3>& ignitionPoints) {
    for (auto& p : particles) {
        if (p.type != TYPE_FABRIC && p.type != TYPE_WOOD && p.type != TYPE_CARPET) {
            if (p.type >= TYPE_PAPER) {
                p.velocity.w = 9999.0f;
            }
            continue;
        }

        glm::vec3 pos(p.position);
        float minD = 1000.0f;
        for (const glm::vec3& ip : ignitionPoints) {
            glm::vec3 d = pos - ip;
            float anisotropic = std::sqrt(d.x * d.x * 0.75f + d.y * d.y * 1.55f + d.z * d.z * 0.95f);
            minD = std::min(minD, anisotropic);
        }

        float grain = hash01(pos * 5.0f);
        float verticalPenalty = std::max(0.0f, pos.y - 0.92f) * 0.85f;

        if (p.type == TYPE_FABRIC) {
            p.velocity.w = 0.35f + minD * 5.2f + verticalPenalty + grain * 1.10f;
        } else if (p.type == TYPE_CARPET) {
            p.velocity.w = 1.2f + minD * 8.0f + grain * 1.8f;
        } else {
            p.velocity.w = 3.2f + minD * 11.0f + verticalPenalty * 1.9f + grain * 3.0f;
        }
    }
}

class ChairBurnScene : public Scene {
public:
    std::string getName() const override {
        return "Silla 3D Quemando";
    }

    float getDuration() const override {
        return 24.0f;
    }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(0.12f, 0.125f, 0.13f);
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {
        emitters.clear();

        const float pSize = lightweight ? 0.0095f : 0.0062f;
        const glm::vec4 fabricColor(0.58f, 0.50f, 0.43f, 1.0f);
        const glm::vec4 seamColor(0.46f, 0.39f, 0.34f, 1.0f);
        const glm::vec4 woodColor(0.36f, 0.20f, 0.10f, 1.0f);
        const glm::vec4 carpetColor(0.44f, 0.42f, 0.37f, 1.0f);
        const glm::vec4 metalColor(0.18f, 0.17f, 0.16f, 1.0f);

        int dense = lightweight ? 1 : 2;

        GeometryUtils::generatePlane(
            particles,
            glm::vec3(0.0f, 0.005f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f), 4.2f, 180 * dense,
            glm::vec3(0.0f, 0.0f, 1.0f), 3.6f, 150 * dense,
            TYPE_CARPET,
            carpetColor,
            pSize * 1.15f
        );

        GeometryUtils::generateBox(
            particles,
            glm::vec3(0.0f, 0.86f, 0.02f),
            glm::vec3(1.34f, 0.18f, 1.08f),
            120 * dense, 12 * dense, 96 * dense,
            TYPE_FABRIC,
            fabricColor,
            pSize
        );

        GeometryUtils::generateBox(
            particles,
            glm::vec3(0.0f, 1.46f, -0.50f),
            glm::vec3(1.34f, 1.12f, 0.18f),
            118 * dense, 90 * dense, 12 * dense,
            TYPE_FABRIC,
            fabricColor,
            pSize
        );

        for (float x : {-0.48f, 0.0f, 0.48f}) {
            GeometryUtils::generateBox(
                particles,
                glm::vec3(x, 1.46f, -0.405f),
                glm::vec3(0.035f, 1.04f, 0.022f),
                4 * dense, 80 * dense, 3 * dense,
                TYPE_FABRIC,
                seamColor,
                pSize * 0.85f
            );
        }

        for (float x : {-0.56f, 0.56f}) {
            for (float z : {-0.42f, 0.42f}) {
                GeometryUtils::generateCylinder(
                    particles,
                    glm::vec3(x, 0.08f, z),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    0.055f,
                    0.78f,
                    18 * dense,
                    54 * dense,
                    TYPE_WOOD,
                    woodColor,
                    pSize * 1.05f
                );
            }
        }

        GeometryUtils::generateBox(particles, glm::vec3(0.0f, 0.58f, 0.48f), glm::vec3(1.24f, 0.075f, 0.08f), 90 * dense, 8 * dense, 8 * dense, TYPE_WOOD, woodColor, pSize);
        GeometryUtils::generateBox(particles, glm::vec3(0.0f, 0.58f, -0.48f), glm::vec3(1.24f, 0.075f, 0.08f), 90 * dense, 8 * dense, 8 * dense, TYPE_WOOD, woodColor, pSize);
        GeometryUtils::generateBox(particles, glm::vec3(-0.64f, 0.58f, 0.0f), glm::vec3(0.08f, 0.075f, 1.02f), 8 * dense, 8 * dense, 72 * dense, TYPE_WOOD, woodColor, pSize);
        GeometryUtils::generateBox(particles, glm::vec3(0.64f, 0.58f, 0.0f), glm::vec3(0.08f, 0.075f, 1.02f), 8 * dense, 8 * dense, 72 * dense, TYPE_WOOD, woodColor, pSize);

        GeometryUtils::generateBox(particles, glm::vec3(-0.68f, 1.45f, -0.50f), glm::vec3(0.09f, 1.40f, 0.10f), 8 * dense, 95 * dense, 8 * dense, TYPE_WOOD, woodColor, pSize);
        GeometryUtils::generateBox(particles, glm::vec3(0.68f, 1.45f, -0.50f), glm::vec3(0.09f, 1.40f, 0.10f), 8 * dense, 95 * dense, 8 * dense, TYPE_WOOD, woodColor, pSize);
        GeometryUtils::generateBox(particles, glm::vec3(0.0f, 2.08f, -0.50f), glm::vec3(1.46f, 0.09f, 0.10f), 105 * dense, 8 * dense, 8 * dense, TYPE_WOOD, woodColor, pSize);

        GeometryUtils::generateBox(particles, glm::vec3(-0.73f, 0.68f, 0.0f), glm::vec3(0.08f, 0.18f, 1.08f), 8 * dense, 14 * dense, 80 * dense, TYPE_WOOD, woodColor, pSize);
        GeometryUtils::generateBox(particles, glm::vec3(0.73f, 0.68f, 0.0f), glm::vec3(0.08f, 0.18f, 1.08f), 8 * dense, 14 * dense, 80 * dense, TYPE_WOOD, woodColor, pSize);

        GeometryUtils::generateBox(
            particles,
            glm::vec3(0.0f, 0.04f, 0.0f),
            glm::vec3(1.65f, 0.035f, 1.35f),
            90 * dense, 3 * dense, 75 * dense,
            TYPE_METAL_GLASS,
            metalColor,
            pSize * 0.78f
        );

        std::vector<glm::vec3> ignitionPoints = {
            glm::vec3(-0.44f, 0.97f, 0.38f),
            glm::vec3(-0.25f, 0.94f, 0.48f)
        };
        assignChairBurnTimes(particles, ignitionPoints);

        EmitterConfig starter;
        starter.position = ignitionPoints.front();
        starter.shape = EmitterShape::DISK;
        starter.radius = 0.045f;
        starter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
        starter.emitRate = lightweight ? 700.0f : 1300.0f;
        starter.particleLife = 0.55f;
        starter.initialSpeed = 0.35f;
        starter.speedVariance = 0.22f;
        starter.temperature = 0.95f;
        starter.particleSize = 0.018f;
        emitters.push_back(starter);

        curlParams.frequency = 2.45f;
        curlParams.amplitude = 1.25f;
        curlParams.octaves = 4;
        curlParams.lacunarity = 2.0f;
        curlParams.persistence = 0.48f;
        curlParams.timeScale = 1.0f;
        curlParams.epsilon = 0.01f;

        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        glm::vec3 target(0.0f, 1.03f, -0.03f);
        cameraPath.keyframes.push_back({0.0f, glm::vec3(2.25f, 1.38f, 2.85f), target});
        cameraPath.keyframes.push_back({8.0f, glm::vec3(1.65f, 1.36f, 2.45f), target});
        cameraPath.keyframes.push_back({16.0f, glm::vec3(1.05f, 1.45f, 2.25f), target});
        cameraPath.keyframes.push_back({24.0f, glm::vec3(0.78f, 1.55f, 2.15f), target});
    }
};

} // namespace

std::unique_ptr<Scene> createChairBurnScene() {
    return std::make_unique<ChairBurnScene>();
}
