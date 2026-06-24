#include "Test6Scene.h"
#include <iostream>

class Test6SceneImpl : public Scene {
public:
    std::string getName() const override {
        return "Prueba de Cemento Incombustible";
    }

    float getDuration() const override {
        return 15.0f;
    }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(0.15f, 0.15f, 0.15f); // Fondo oscuro para resaltar el fuego
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {
        
        particles.clear();
        emitters.clear();

        // Color GRIS para el bloque de cemento
        glm::vec4 cementColor(0.5f, 0.5f, 0.5f, 1.0f);

        // Densidad del bloque
        int blockX = lightweight ? 50 : 80;
        int blockY = lightweight ? 50 : 80;
        int blockZ = lightweight ? 50 : 80;

        float particleSize = lightweight ? 0.015f : 0.01f;

        // Generar bloque de cemento en el centro
        glm::vec3 center(0.0f, 1.0f, 0.0f);
        glm::vec3 size(1.5f, 1.5f, 1.5f);
        GeometryUtils::generateBox(
            particles,
            center, size,
            blockX, blockY, blockZ,
            TYPE_CEMENT,
            cementColor,
            particleSize
        );

        // Calculamos el tiempo de calentamiento del cemento en base a su proximidad al fuego (que está abajo)
        glm::vec3 fireSource = center - glm::vec3(0.0f, 1.0f, 0.0f);
        
        for (auto& p : particles) {
            if (p.type == TYPE_CEMENT) {
                float distToFire = glm::distance(glm::vec3(p.position), fireSource);
                // El calor sube progresivamente: las partes más cercanas se "calientan" (ensucian) en 1 seg,
                // las partes más altas o lejanas tardan hasta 8 segundos en mancharse.
                p.velocity.w = distToFire * 3.5f; 
            }
        }

        // Emisores de fuego continuo DEBAJO y ALREDEDOR del bloque para intentar quemarlo
        EmitterConfig fire1;
        fire1.position = center - glm::vec3(0.0f, 1.0f, 0.0f); // Debajo
        fire1.shape = EmitterShape::DISK;
        fire1.radius = 0.8f;
        fire1.emitRate = lightweight ? 3000.0f : 8000.0f;
        fire1.particleLife = 1.5f;
        fire1.initialSpeed = 2.0f;
        fire1.direction = glm::vec3(0.0f, 1.0f, 0.0f); // Fuego hacia arriba
        emitters.push_back(fire1);

        // Configuración de Curl Noise para simular cómo el fuego "abraza" e intenta subir por el bloque
        curlParams.frequency = 1.5f;
        curlParams.amplitude = 2.5f; 
        curlParams.octaves = 3;
        curlParams.lacunarity = 2.0f;
        curlParams.persistence = 0.5f;
        curlParams.timeScale = 1.0f;
        curlParams.epsilon = 0.01f;

        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        cameraPath.keyframes.push_back({0.0f, glm::vec3(0.0f, 2.0f, 4.0f), center});
        cameraPath.keyframes.push_back({7.5f, glm::vec3(-3.0f, 2.0f, 3.0f), center});
        cameraPath.keyframes.push_back({15.0f, glm::vec3(0.0f, 2.0f, 4.0f), center});
    }
};

std::unique_ptr<Scene> createTest6Scene() {
    return std::make_unique<Test6SceneImpl>();
}
