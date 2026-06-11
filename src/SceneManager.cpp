// #include "SceneManager.h"
// #include <iostream>

// // =============================================================================
// // Scene 0: Fogata Simple
// // =============================================================================
// class Scene0_Campfire : public Scene {
// public:
//     std::string getName() const override { return "Fogata Simple"; }
//     float getDuration() const override { return 10.0f; }
//     glm::vec3 getBackgroundColor() const override { return glm::vec3(0.01f); }

//     void setup(std::vector<Particle>& particles, std::vector<EmitterConfig>& emitters,
//                CurlNoiseParams& curlParams, CameraPath& cameraPath, bool lightweight) override {
//         // Emitter
//         EmitterConfig em;
//         em.position = glm::vec3(0.0f, 0.0f, 0.0f);
//         em.shape = EmitterShape::DISK;
//         em.radius = 0.3f;
//         em.direction = glm::vec3(0.0f, 1.0f, 0.0f);
//         em.emitRate = lightweight ? 2000.0f : 8000.0f;
//         em.particleLife = 1.5f;
//         em.initialSpeed = 1.5f;
//         emitters.push_back(em);

//         // Curl Noise
//         curlParams.frequency = 1.2f;
//         curlParams.amplitude = 1.5f;

//         // Camera
//         cameraPath.totalDuration = getDuration();
//         cameraPath.keyframes.push_back({0.0f, glm::vec3(0.0f, 2.0f, 4.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
//         cameraPath.keyframes.push_back({5.0f, glm::vec3(3.0f, 1.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
//         cameraPath.keyframes.push_back({10.0f, glm::vec3(0.0f, 2.0f, 4.0f), glm::vec3(0.0f, 1.0f, 0.0f)});

//         // Partículas estáticas (madera base)
//         GeometryUtils::generateCylinder(particles, glm::vec3(0,0,0), glm::vec3(0,1,0), 0.5f, 0.2f, 10, 3,
//                                         TYPE_WOOD, glm::vec4(0.4f, 0.2f, 0.1f, 1.0f), 0.05f);
//     }
// };

// // =============================================================================
// // Scene 1: Pared de Fuego
// // =============================================================================
// class Scene1_FireWall : public Scene {
// public:
//     std::string getName() const override { return "Pared de Fuego"; }
//     float getDuration() const override { return 12.0f; }

//     void setup(std::vector<Particle>& particles, std::vector<EmitterConfig>& emitters,
//                CurlNoiseParams& curlParams, CameraPath& cameraPath, bool lightweight) override {
//         EmitterConfig em;
//         em.position = glm::vec3(0.0f, -1.0f, 0.0f);
//         em.shape = EmitterShape::LINE;
//         em.width = 4.0f; // Longitud de la línea
//         em.direction = glm::vec3(0.0f, 1.0f, 0.0f);
//         em.emitRate = lightweight ? 5000.0f : 20000.0f;
//         emitters.push_back(em);

//         curlParams.frequency = 2.0f;
//         curlParams.amplitude = 2.5f;

//         cameraPath.totalDuration = getDuration();
//         cameraPath.keyframes.push_back({0.0f, glm::vec3(0.0f, 1.0f, 6.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
//         cameraPath.keyframes.push_back({6.0f, glm::vec3(-3.0f, 2.0f, 4.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
//         cameraPath.keyframes.push_back({12.0f, glm::vec3(0.0f, 1.0f, 6.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
//     }
// };

// // =============================================================================
// // Scene 2: Objeto Ardiendo
// // =============================================================================
// class Scene2_BurningObject : public Scene {
// public:
//     std::string getName() const override { return "Objeto Ardiendo"; }
//     float getDuration() const override { return 15.0f; }

//     void setup(std::vector<Particle>& particles, std::vector<EmitterConfig>& emitters,
//                CurlNoiseParams& curlParams, CameraPath& cameraPath, bool lightweight) override {
        
//         // Objeto de papel
//         int count = lightweight ? 50 : 150;
//         GeometryUtils::generatePlane(particles, glm::vec3(0, 1, 0),
//                                      glm::vec3(1,0,0), 2.0f, count,
//                                      glm::vec3(0,1,0), 2.0f, count,
//                                      TYPE_PAPER, glm::vec4(0.9f, 0.9f, 0.8f, 1.0f), 0.02f);
        
//         std::vector<glm::vec3> ignitions = { glm::vec3(0.0f, 0.0f, 0.0f) };
//         GeometryUtils::computeBurnTimes(particles, ignitions, 0.5f, 0.1f);

//         // Emitter base opcional
//         EmitterConfig em;
//         em.position = glm::vec3(0.0f, 0.0f, 0.0f);
//         em.shape = EmitterShape::POINT;
//         em.emitRate = lightweight ? 1000.0f : 3000.0f;
//         emitters.push_back(em);

//         curlParams.frequency = 1.5f;
//         curlParams.amplitude = 1.0f;

//         cameraPath.totalDuration = getDuration();
//         cameraPath.keyframes.push_back({0.0f, glm::vec3(0.0f, 1.0f, 4.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
//         cameraPath.keyframes.push_back({15.0f, glm::vec3(0.0f, 1.0f, 4.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
//     }
// };

// // =============================================================================
// // Scene 3: Explosión
// // =============================================================================
// class Scene3_Explosion : public Scene {
// public:
//     std::string getName() const override { return "Explosion"; }
//     float getDuration() const override { return 5.0f; }

//     void setup(std::vector<Particle>& particles, std::vector<EmitterConfig>& emitters,
//                CurlNoiseParams& curlParams, CameraPath& cameraPath, bool lightweight) override {
//         EmitterConfig em;
//         em.position = glm::vec3(0.0f, 1.0f, 0.0f);
//         em.shape = EmitterShape::SPHERE;
//         em.radius = 0.5f;
//         em.emitRate = lightweight ? 20000.0f : 100000.0f;
//         em.initialSpeed = 5.0f;
//         em.particleLife = 1.0f;
//         emitters.push_back(em);

//         curlParams.frequency = 3.0f;
//         curlParams.amplitude = 5.0f;

//         cameraPath.totalDuration = getDuration();
//         cameraPath.keyframes.push_back({0.0f, glm::vec3(0.0f, 2.0f, 8.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
//         cameraPath.keyframes.push_back({5.0f, glm::vec3(0.0f, 2.0f, 8.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
//     }
// };

// // =============================================================================
// // Scene 4: Tormenta de Fuego
// // =============================================================================
// class Scene4_FireStorm : public Scene {
// public:
//     std::string getName() const override { return "Tormenta de Fuego"; }
//     float getDuration() const override { return 15.0f; }

//     void setup(std::vector<Particle>& particles, std::vector<EmitterConfig>& emitters,
//                CurlNoiseParams& curlParams, CameraPath& cameraPath, bool lightweight) override {
        
//         EmitterConfig em1, em2;
//         em1.position = glm::vec3(-2.0f, 0.0f, 0.0f);
//         em1.shape = EmitterShape::DISK;
//         em1.radius = 0.5f;
//         em1.emitRate = lightweight ? 3000.0f : 8000.0f;
        
//         em2.position = glm::vec3(2.0f, 0.0f, 0.0f);
//         em2.shape = EmitterShape::DISK;
//         em2.radius = 0.5f;
//         em2.emitRate = lightweight ? 3000.0f : 8000.0f;

//         emitters.push_back(em1);
//         emitters.push_back(em2);

//         curlParams.frequency = 2.5f;
//         curlParams.amplitude = 4.0f;
//         curlParams.timeScale = 1.5f;

//         cameraPath.totalDuration = getDuration();
//         cameraPath.keyframes.push_back({0.0f, glm::vec3(0.0f, 3.0f, 7.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
//         cameraPath.keyframes.push_back({7.5f, glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
//         cameraPath.keyframes.push_back({15.0f, glm::vec3(0.0f, 3.0f, 7.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
//     }
// };

// // =============================================================================
// // SceneManager Implementation
// // =============================================================================
// SceneManager::SceneManager() {}
// SceneManager::~SceneManager() {}

// void SceneManager::init() {
//     scenes.push_back(std::make_unique<Scene0_Campfire>());
//     scenes.push_back(std::make_unique<Scene1_FireWall>());
//     scenes.push_back(std::make_unique<Scene2_BurningObject>());
//     scenes.push_back(std::make_unique<Scene3_Explosion>());
//     scenes.push_back(std::make_unique<Scene4_FireStorm>());
// }

// bool SceneManager::loadScene(int index, ParticleSystem& particleSystem, Camera& camera, bool lightweight) {
//     if (index < 0 || index >= scenes.size()) return false;

//     currentSceneIndex = index;
//     Scene* scene = scenes[index].get();

//     std::vector<Particle> initialParticles;
//     std::vector<EmitterConfig> emitters;
//     CurlNoiseParams curlParams;
//     CameraPath cameraPath;

//     // Rellenamos el pool de partículas muertas para emisiones dinámicas
//     int maxPoolSize = lightweight ? 100000 : 500000; 
//     for(int i = 0; i < maxPoolSize; ++i) {
//         Particle p{};
//         p.type = TYPE_DEAD;
//         initialParticles.push_back(p);
//     }

//     // El scene setup puede añadir partículas al pool (e.g. materiales)
//     scene->setup(initialParticles, emitters, curlParams, cameraPath, lightweight);

//     particleSystem.cleanup(); // Clean previous state
//     particleSystem.init(initialParticles, emitters);
//     particleSystem.setCurlNoiseParams(curlParams);

//     // Apply specific wind if needed, here we just reset
//     particleSystem.setWind(glm::vec3(1.0f, 0.0f, 0.0f), index == 4 ? 2.0f : 0.0f);

//     camera.setPath(cameraPath);

//     return true;
// }

// std::string SceneManager::getCurrentSceneName() const {
//     if (currentSceneIndex >= 0 && currentSceneIndex < scenes.size()) {
//         return scenes[currentSceneIndex]->getName();
//     }
//     return "None";
// }

// float SceneManager::getCurrentSceneDuration() const {
//     if (currentSceneIndex >= 0 && currentSceneIndex < scenes.size()) {
//         return scenes[currentSceneIndex]->getDuration();
//     }
//     return 0.0f;
// }

// glm::vec3 SceneManager::getBackgroundColor() const {
//     if (currentSceneIndex >= 0 && currentSceneIndex < scenes.size()) {
//         return scenes[currentSceneIndex]->getBackgroundColor();
//     }
//     return glm::vec3(0.0f);
// }
#include "SceneManager.h"
#include <iostream>
#include <random>
#include "../include/Test5Scene.h"

// =============================================================================
// Scene 0: Hoja de Papel - Papel Quemándose
// =============================================================================
class Scene0_Campfire : public Scene {
public:
    std::string getName() const override {
        return "Hoja de Papel Quemando";
    }

    float getDuration() const override {
        return 20.0f;
    }

    glm::vec3 getBackgroundColor() const override {
        // Fondo gris de estudio para resaltar el contraste
        return glm::vec3(0.25f, 0.25f, 0.25f);
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {

        particles.clear();
        emitters.clear();

        glm::vec4 deskColor(0.28f, 0.16f, 0.08f, 1.0f);
        glm::vec4 paperColor(0.95f, 0.95f, 0.95f, 1.0f); // Papel blanco brillante

        // Mesa: moderadamente densa
        int deskU  = lightweight ? 80 : 150;
        int deskV  = lightweight ? 60 : 100;

        // Papel: Densidad extrema para formar una superficie sólida que parezca papel real
        int paperU = lightweight ? 350 : 500;
        int paperV = lightweight ? 250 : 350;

        float deskParticleSize  = lightweight ? 0.0075f : 0.006f;
        float paperParticleSize = lightweight ? 0.005f : 0.004f;

        // Mesa
        GeometryUtils::generatePlane(
            particles,
            glm::vec3(0.0f, -0.045f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f), 3.2f, deskU,
            glm::vec3(0.0f, 0.0f, 1.0f), 2.2f, deskV,
            TYPE_WOOD,
            deskColor,
            deskParticleSize
        );

        // Papel
        glm::vec3 paperCenter(0.0f, 0.005f, 0.0f);
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
        float baseBurnSpeed = 0.45f;
        
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
                
                // 1. Variación espacial fractal (simula áreas secas y húmedas)
                float noise = sin(pos.x * 15.0f) * cos(pos.z * 15.0f) * 0.5f +
                              sin(pos.x * 35.0f + 1.2f) * cos(pos.z * 35.0f + 0.5f) * 0.25f +
                              dist(rng) * 0.1f;
                              
                // 2. Aceleración térmica (el fuego avanza más rápido a medida que se aleja del punto original porque el aire ya está caliente)
                // Usamos pow para que la distancia reduzca su impacto en el tiempo
                float thermalAcceleration = std::pow(minDist, 0.85f);
                
                // 3. Agujeros dentados: Algunas partes del papel son extremadamente frágiles y se queman casi instantáneamente
                float fragility = (dist(rng) > 0.85f) ? 0.3f : 1.0f;
                
                float burnTime = (thermalAcceleration / baseBurnSpeed) + noise * 0.4f * fragility;
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
        curlParams.amplitude = 1.0f; // Fuego más agresivo
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

// =============================================================================
// Scene 1: Pared de Fuego
// =============================================================================
class Scene1_FireWall : public Scene {
public:
    std::string getName() const override {
        return "Pared de Fuego";
    }

    float getDuration() const override {
        return 12.0f;
    }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(0.02f, 0.02f, 0.02f);
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {

        EmitterConfig em;
        em.position = glm::vec3(0.0f, -1.0f, 0.0f);
        em.shape = EmitterShape::LINE;
        em.width = 4.0f;
        em.direction = glm::vec3(0.0f, 1.0f, 0.0f);
        em.emitRate = lightweight ? 5000.0f : 20000.0f;
        emitters.push_back(em);

        curlParams.frequency = 2.0f;
        curlParams.amplitude = 2.5f;
        curlParams.octaves = 3;
        curlParams.lacunarity = 2.0f;
        curlParams.persistence = 0.5f;
        curlParams.timeScale = 1.0f;
        curlParams.epsilon = 0.01f;

        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();

        cameraPath.keyframes.push_back({
            0.0f,
            glm::vec3(0.0f, 1.0f, 6.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        });

        cameraPath.keyframes.push_back({
            6.0f,
            glm::vec3(-3.0f, 2.0f, 4.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        });

        cameraPath.keyframes.push_back({
            12.0f,
            glm::vec3(0.0f, 1.0f, 6.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        });
    }
};

// =============================================================================
// Scene 2: Objeto Ardiendo
// =============================================================================
class Scene2_BurningObject : public Scene {
public:
    std::string getName() const override {
        return "Objeto Ardiendo";
    }

    float getDuration() const override {
        return 15.0f;
    }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(0.02f, 0.02f, 0.02f);
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {

        int count = lightweight ? 50 : 150;

        GeometryUtils::generatePlane(
            particles,
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f), 2.0f, count,
            glm::vec3(0.0f, 1.0f, 0.0f), 2.0f, count,
            TYPE_PAPER,
            glm::vec4(0.9f, 0.9f, 0.8f, 1.0f),
            0.02f
        );

        std::vector<glm::vec3> ignitions = {
            glm::vec3(0.0f, 0.0f, 0.0f)
        };

        GeometryUtils::computeBurnTimes(particles, ignitions, 0.5f, 0.1f);

        EmitterConfig em;
        em.position = glm::vec3(0.0f, 0.0f, 0.0f);
        em.shape = EmitterShape::POINT;
        em.emitRate = lightweight ? 1000.0f : 3000.0f;
        emitters.push_back(em);

        curlParams.frequency = 1.5f;
        curlParams.amplitude = 1.0f;
        curlParams.octaves = 3;
        curlParams.lacunarity = 2.0f;
        curlParams.persistence = 0.5f;
        curlParams.timeScale = 1.0f;
        curlParams.epsilon = 0.01f;

        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();

        cameraPath.keyframes.push_back({
            0.0f,
            glm::vec3(0.0f, 1.0f, 4.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        });

        cameraPath.keyframes.push_back({
            15.0f,
            glm::vec3(0.0f, 1.0f, 4.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        });
    }
};

// =============================================================================
// Scene 3: Explosión
// =============================================================================
class Scene3_Explosion : public Scene {
public:
    std::string getName() const override {
        return "Explosion";
    }

    float getDuration() const override {
        return 5.0f;
    }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(0.01f, 0.01f, 0.01f);
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {

        EmitterConfig em;
        em.position = glm::vec3(0.0f, 1.0f, 0.0f);
        em.shape = EmitterShape::SPHERE;
        em.radius = 0.5f;
        em.emitRate = lightweight ? 20000.0f : 100000.0f;
        em.initialSpeed = 5.0f;
        em.particleLife = 1.0f;
        emitters.push_back(em);

        curlParams.frequency = 3.0f;
        curlParams.amplitude = 5.0f;
        curlParams.octaves = 4;
        curlParams.lacunarity = 2.0f;
        curlParams.persistence = 0.55f;
        curlParams.timeScale = 1.2f;
        curlParams.epsilon = 0.01f;

        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();

        cameraPath.keyframes.push_back({
            0.0f,
            glm::vec3(0.0f, 2.0f, 8.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        });

        cameraPath.keyframes.push_back({
            5.0f,
            glm::vec3(0.0f, 2.0f, 8.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        });
    }
};

// =============================================================================
// Scene 4: Tormenta de Fuego
// =============================================================================
class Scene4_FireStorm : public Scene {
public:
    std::string getName() const override {
        return "Tormenta de Fuego";
    }

    float getDuration() const override {
        return 15.0f;
    }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(0.02f, 0.02f, 0.02f);
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {

        EmitterConfig em1;
        EmitterConfig em2;

        em1.position = glm::vec3(-2.0f, 0.0f, 0.0f);
        em1.shape = EmitterShape::DISK;
        em1.radius = 0.5f;
        em1.emitRate = lightweight ? 3000.0f : 8000.0f;

        em2.position = glm::vec3(2.0f, 0.0f, 0.0f);
        em2.shape = EmitterShape::DISK;
        em2.radius = 0.5f;
        em2.emitRate = lightweight ? 3000.0f : 8000.0f;

        emitters.push_back(em1);
        emitters.push_back(em2);

        curlParams.frequency = 2.5f;
        curlParams.amplitude = 4.0f;
        curlParams.octaves = 4;
        curlParams.lacunarity = 2.0f;
        curlParams.persistence = 0.55f;
        curlParams.timeScale = 1.5f;
        curlParams.epsilon = 0.01f;

        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();

        cameraPath.keyframes.push_back({
            0.0f,
            glm::vec3(0.0f, 3.0f, 7.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        });

        cameraPath.keyframes.push_back({
            7.5f,
            glm::vec3(5.0f, 3.0f, 5.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        });

        cameraPath.keyframes.push_back({
            15.0f,
            glm::vec3(0.0f, 3.0f, 7.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        });
    }
};

// =============================================================================
// SceneManager Implementation
// =============================================================================
SceneManager::SceneManager() {}

SceneManager::~SceneManager() {}

void SceneManager::init() {
    scenes.clear();

    scenes.push_back(std::make_unique<Scene0_Campfire>());
    scenes.push_back(std::make_unique<Scene1_FireWall>());
    scenes.push_back(std::make_unique<Scene2_BurningObject>());
    scenes.push_back(std::make_unique<Scene3_Explosion>());
    scenes.push_back(std::make_unique<Scene4_FireStorm>());
    scenes.push_back(createTest5Scene());
}

bool SceneManager::loadScene(int index,
                             ParticleSystem& particleSystem,
                             Camera& camera,
                             bool lightweight) {
    if (index < 0 || index >= static_cast<int>(scenes.size())) {
        return false;
    }

    currentSceneIndex = index;

    Scene* scene = scenes[index].get();

    std::vector<Particle> initialParticles;
    std::vector<EmitterConfig> emitters;
    CurlNoiseParams curlParams;
    CameraPath cameraPath;

    // Permitir pool de partículas muertas para TODAS las escenas,
    // porque el papel quemándose necesita espacio para emitir las llamas (TYPE_FIRE).
    int maxPoolSize = lightweight ? 100000 : 500000;

    initialParticles.reserve(maxPoolSize);

    for (int i = 0; i < maxPoolSize; ++i) {
        Particle p{};
        p.type = TYPE_DEAD;
        p.position = glm::vec4(0.0f);
        p.velocity = glm::vec4(0.0f);
        p.color = glm::vec4(0.0f);
        p.lifetime = 0.0f;
        p.maxLifetime = 1.0f;
        p.temperature = 0.0f;

        initialParticles.push_back(p);
    }

    scene->setup(
        initialParticles,
        emitters,
        curlParams,
        cameraPath,
        lightweight
    );

    particleSystem.cleanup();
    particleSystem.init(initialParticles, emitters);
    particleSystem.setCurlNoiseParams(curlParams);

    particleSystem.setWind(
        glm::vec3(1.0f, 0.0f, 0.0f),
        index == 4 ? 2.0f : 0.0f
    );

    camera.setPath(cameraPath);

    std::cout << "[SceneManager] Loaded scene " << index
              << " - " << scene->getName()
              << " | particles: " << initialParticles.size()
              << " | emitters: " << emitters.size()
              << std::endl;

    return true;
}

std::string SceneManager::getCurrentSceneName() const {
    if (currentSceneIndex >= 0 && currentSceneIndex < static_cast<int>(scenes.size())) {
        return scenes[currentSceneIndex]->getName();
    }

    return "None";
}

float SceneManager::getCurrentSceneDuration() const {
    if (currentSceneIndex >= 0 && currentSceneIndex < static_cast<int>(scenes.size())) {
        return scenes[currentSceneIndex]->getDuration();
    }

    return 0.0f;
}

glm::vec3 SceneManager::getBackgroundColor() const {
    if (currentSceneIndex >= 0 && currentSceneIndex < static_cast<int>(scenes.size())) {
        return scenes[currentSceneIndex]->getBackgroundColor();
    }

    return glm::vec3(0.0f);
}