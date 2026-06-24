#include "Test7Scene.h"
#include <iostream>
#include <random>

class Test7SceneImpl : public Scene {
public:
    std::string getName() const override {
        return "Escena 7: Arbol quemandose (Hiperrealista Lento)";
    }

    float getDuration() const override {
        return 120.0f; // Simulación de 2 minutos para apreciar el fuego lento
    }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(1.0f, 1.0f, 1.0f); // BLANCO TOTAL
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {
        
        particles.clear();
        emitters.clear();

        glm::vec4 woodColor(0.4f, 0.25f, 0.15f, 1.0f);
        glm::vec4 leafColor(0.12f, 0.45f, 0.12f, 1.0f); // Hojas 100% sólidas
        glm::vec4 groundColor(0.1f, 0.2f, 0.1f, 1.0f);

        float pSize = lightweight ? 0.012f : 0.008f;
        // Densidad extrema para el tronco
        int trunkRadial = lightweight ? 80 : 150;
        int trunkHeight = lightweight ? 150 : 350;
        // Densidad extrema para la copa del árbol
        int leafCount = lightweight ? 50000 : 150000; 
        int groundRes = lightweight ? 100 : 200;

        // 1. Suelo (No se quema, usamos lógica de cemento)
        GeometryUtils::generatePlane(
            particles,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f), 4.0f, groundRes,
            glm::vec3(0.0f, 0.0f, 1.0f), 4.0f, groundRes,
            TYPE_CEMENT, groundColor, pSize
        );

        // 2. Tronco del árbol
        glm::vec3 trunkBase(0.0f, 0.0f, 0.0f);
        GeometryUtils::generateCylinder(
            particles,
            trunkBase, glm::vec3(0.0f, 1.0f, 0.0f),
            0.25f, 2.5f, trunkRadial, trunkHeight,
            TYPE_WOOD, woodColor, pSize
        );

        // 3. Hojas (Copa del árbol)
        GeometryUtils::generateSphere(
            particles,
            glm::vec3(0.0f, 2.3f, 0.0f), 1.5f, leafCount,
            TYPE_LEAF, leafColor, pSize
        );
        GeometryUtils::generateSphere(
            particles,
            glm::vec3(0.8f, 1.8f, 0.5f), 1.0f, leafCount/2,
            TYPE_LEAF, leafColor, pSize
        );
        GeometryUtils::generateSphere(
            particles,
            glm::vec3(-0.8f, 1.9f, -0.4f), 1.2f, leafCount/2,
            TYPE_LEAF, leafColor, pSize
        );

        // PUNTO DE IGNICIÓN: Fuego en la base del tronco central
        glm::vec3 fireSource(0.0f, 0.1f, 0.0f);
        
        std::mt19937 rng(1337);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f); // Ahora solo números positivos

        // Configurar tiempos de ignición basados en convección térmica realista
        for (auto& p : particles) {
            if (p.type == TYPE_CEMENT) {
                float d = glm::distance(glm::vec3(p.position), fireSource);
                p.velocity.w = d * 5.0f; 
            } else if (p.type == TYPE_WOOD || p.type == TYPE_LEAF) {
                
                float dx = p.position.x - fireSource.x;
                float dy = std::max(0.0f, p.position.y - fireSource.y);
                float dz = p.position.z - fireSource.z;
                
                // CONVECCIÓN TÉRMICA
                float perceivedDist = sqrt(dx*dx*3.0f + dy*dy*0.6f + dz*dz*3.0f);
                
                float delayNoise = std::abs(sin(p.position.y * 4.0f + p.position.x * 3.0f)) * 3.0f;
                float chaos = dist(rng) * 1.5f;
                
                // RESISTENCIA EXTREMA: Un árbol real tarda MUCHO en prender.
                // Aquí la propagación tomará decenas de segundos en llegar arriba.
                float resistance = (p.type == TYPE_WOOD) ? 150.0f : 120.0f;
                
                float burnTime = (perceivedDist * resistance) + delayNoise + chaos;
                
                p.velocity.w = std::max(0.0f, burnTime);
            }
        }

        // Emisor de la fogata base (crea el fuego inicial)
        EmitterConfig fire1;
        fire1.position = fireSource;
        fire1.shape = EmitterShape::DISK;
        fire1.radius = 0.3f;
        fire1.emitRate = lightweight ? 1000.0f : 2500.0f;
        fire1.particleLife = 1.2f;
        fire1.initialSpeed = 1.5f;
        fire1.direction = glm::vec3(0.0f, 1.0f, 0.0f);
        emitters.push_back(fire1);

        // Curl Noise orgánico para llamas forestales
        curlParams.frequency = 1.2f;
        curlParams.amplitude = 1.8f; 
        curlParams.octaves = 3;
        curlParams.lacunarity = 2.0f;
        curlParams.persistence = 0.5f;
        curlParams.timeScale = 1.2f;
        curlParams.epsilon = 0.01f;

        // Cámara rotando lentamente alrededor del árbol
        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        glm::vec3 target(0.0f, 2.0f, 0.0f);
        cameraPath.keyframes.push_back({0.0f, glm::vec3(0.0f, 2.0f, 8.0f), target});
        cameraPath.keyframes.push_back({60.0f, glm::vec3(8.0f, 5.0f, 0.0f), target});
        cameraPath.keyframes.push_back({120.0f, glm::vec3(0.0f, 2.0f, -8.0f), target});
    }
};

std::unique_ptr<Scene> createTest7Scene() {
    return std::make_unique<Test7SceneImpl>();
}
