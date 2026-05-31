#include "Scene.h"

class BuildingFireScene : public Scene {
public:
    std::string getName() const override { return "BuildingFire"; }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight = false) override {
        
        particles.clear();
        emitters.clear();

        glm::vec4 cementColor(0.6f, 0.58f, 0.55f, 1.0f);
        size_t deadPoolSize = lightweight ? 60000 : 300000;
        float pSize = lightweight ? 0.1f : 0.05f;

        // Facade dimensions
        int floors = 5;
        int windowsPerFloor = 4;
        float width = 10.0f;
        float height = 10.0f;
        
        int gridU = lightweight ? 50 : 150;
        int gridV = lightweight ? 50 : 150;

        float du = width / (gridU - 1);
        float dv = height / (gridV - 1);
        glm::vec3 corner(-width/2.0f, 0.0f, 0.0f);

        // Generate facade with windows (gaps)
        for (int i = 0; i < gridU; ++i) {
            for (int j = 0; j < gridV; ++j) {
                float x = corner.x + i * du;
                float y = corner.y + j * dv;

                // Check if in window area
                bool isWindow = false;
                for(int f=0; f<floors; ++f) {
                    float floorY = 1.0f + f * 2.0f;
                    if (y > floorY && y < floorY + 1.2f) {
                        for(int w=0; w<windowsPerFloor; ++w) {
                            float winX = -4.0f + w * 2.5f;
                            if (x > winX && x < winX + 1.5f) {
                                isWindow = true;
                                break;
                            }
                        }
                    }
                    if (isWindow) break;
                }

                if (!isWindow) {
                    Particle p{};
                    p.position = glm::vec4(x, y, 0.0f, pSize);
                    p.velocity = glm::vec4(0.0f, 0.0f, 0.0f, 1000.0f); // Doesn't burn
                    p.color = cementColor;
                    p.lifetime = 10000.0f;
                    p.maxLifetime = 10000.0f;
                    p.temperature = 0.0f;
                    p.type = TYPE_CEMENT;
                    particles.push_back(p);
                }
            }
        }

        // Add dead particles
        for (size_t i = 0; i < deadPoolSize; ++i) {
            Particle p{};
            p.type = TYPE_DEAD;
            particles.push_back(p);
        }

        curlParams.frequency = 0.8f;
        curlParams.amplitude = 2.5f;
        curlParams.octaves = 4;
        curlParams.timeScale = 0.8f;

        // Multiple interior fires per window row to simulate internal inferno
        for(int f=1; f<3; ++f) { // Fire in floor 1 and 2
            float floorY = 1.2f + f * 2.0f;
            EmitterConfig floorFire;
            floorFire.position = glm::vec3(0.0f, floorY, -1.0f); // Inside the building
            floorFire.direction = glm::vec3(0.0f, 1.0f, 1.0f);   // Fire pushing out of windows
            floorFire.shape = EmitterShape::LINE;
            floorFire.width = 8.0f;
            floorFire.emitRate = lightweight ? 10000.0f : 30000.0f;
            floorFire.particleLife = 2.5f;
            floorFire.initialSpeed = 3.0f;
            floorFire.particleSize = 0.08f;
            emitters.push_back(floorFire);
        }

        // Roof smoke
        EmitterConfig roofSmoke;
        roofSmoke.position = glm::vec3(0.0f, height + 0.5f, 0.0f);
        roofSmoke.direction = glm::vec3(0.0f, 1.0f, 0.0f);
        roofSmoke.shape = EmitterShape::RECTANGLE;
        roofSmoke.width = width * 0.8f;
        roofSmoke.height = 2.0f;
        roofSmoke.emitRate = lightweight ? 5000.0f : 20000.0f;
        roofSmoke.particleLife = 4.0f;
        roofSmoke.initialSpeed = 1.5f;
        roofSmoke.particleSize = 0.15f;
        // Adjust for smoke type emission inside compute shader, but currently emitter spawns fire.
        // It will turn into smoke naturally if fire lifetime is short, so let's lower temperature.
        roofSmoke.temperature = 0.5f; 
        emitters.push_back(roofSmoke);

        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        cameraPath.keyframes.push_back({0.0f, glm::vec3(0.0f, 4.0f, 18.0f), glm::vec3(0.0f, 4.0f, 0.0f)});
        cameraPath.keyframes.push_back({15.0f, glm::vec3(0.0f, 4.0f, 18.0f), glm::vec3(0.0f, 4.0f, 0.0f)}); // hold
        cameraPath.keyframes.push_back({30.0f, glm::vec3(0.0f, 5.0f, 10.0f), glm::vec3(0.0f, 5.0f, 0.0f)}); // slow zoom
    }

    float getDuration() const override { return 30.0f; }
};
