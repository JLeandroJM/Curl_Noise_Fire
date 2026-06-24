// =============================================================================
// TreeScene.cpp - Árbol ardiendo
//
// Comportamiento pedido:
//   - Las HOJAS se incendian de forma DISTRIBUIDA (no en un solo punto) y
//     van DESAPARECIENDO conforme el fuego las consume.
//   - Luego arde el TRONCO (madera, lento).
//   - Que se vea real.
//
// Cómo se logra:
//   - El fuego de la copa NACE DE LAS HOJAS al arder (rama TYPE_LEAF en
//     particle_update.comp: cada hoja llamea, emite fuego/humo, se encoge y
//     muere). Como cada hoja es una fuente, el fuego es distribuido por sí solo.
//   - Las hojas encienden con burnStart aleatorio repartido por toda la copa.
//   - El tronco enciende MÁS TARDE (de arriba hacia abajo) y su cuerpo de
//     llama lo dan emisores a lo largo del tronco, modulados por getEmitScale
//     (arranca cuando ya ardieron las hojas).
// =============================================================================

#include "TreeScene.h"
#include <cmath>
#include <random>
#include <vector>

namespace {

class TreeScene : public Scene {
public:
    std::string getName() const override { return "Arbol Ardiendo"; }

    float getDuration() const override { return 24.0f; }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(0.02f, 0.02f, 0.025f);
    }

    // La envolvente temporal del fuego la maneja cada emisor con su propia
    // ventana (copa primero, tronco después), no una escala global.

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {

        emitters.clear();

        std::mt19937 rng(1234);
        std::uniform_real_distribution<float> r01(0.0f, 1.0f);
        auto rnd = [&](float a, float b) { return a + (b - a) * r01(rng); };

        glm::vec4 woodColor(0.30f, 0.16f, 0.06f, 1.0f);
        glm::vec4 leafColor(0.16f, 0.52f, 0.12f, 1.0f);
        glm::vec4 groundColor(0.07f, 0.075f, 0.055f, 1.0f);

        const float trunkH = 3.0f;
        const float pSize  = lightweight ? 0.05f : 0.026f;
        const float leafPSize = lightweight ? 0.026f : 0.015f;   // hojas finas

        // ------------------------------ Suelo --------------------------------
        int gc = lightweight ? 60 : 110;
        GeometryUtils::generatePlane(particles, glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(1, 0, 0), 7.0f, gc,
                                     glm::vec3(0, 0, 1), 7.0f, gc,
                                     TYPE_GROUND, groundColor, lightweight ? 0.05f : 0.03f);

        // ------------------------------ Tronco -------------------------------
        int radial = lightweight ? 8 : 16;
        int hcount = lightweight ? 60 : 150;
        GeometryUtils::generateCylinder(particles, glm::vec3(0.0f, 0.0f, 0.0f),
                                        glm::vec3(0, 1, 0), 0.16f, trunkH,
                                        radial, hcount, TYPE_WOOD, woodColor, pSize);

        // ------------------------------ Ramas --------------------------------
        std::vector<glm::vec3> canopyCenters;
        int branchParticles = lightweight ? 120 : 360;
        for (int i = 0; i < 6; ++i) {
            float angle = i * 1.047f + rnd(-0.2f, 0.2f);
            float h = 1.6f + (i % 3) * 0.45f;
            glm::vec3 start(0.0f, h, 0.0f);
            glm::vec3 end = start + glm::vec3(std::cos(angle), 0.55f, std::sin(angle)) * rnd(1.3f, 1.7f);
            GeometryUtils::generateLine(particles, start, end, branchParticles, TYPE_WOOD, woodColor, pSize);
            canopyCenters.push_back(end);
        }
        canopyCenters.push_back(glm::vec3(0.0f, trunkH + 0.25f, 0.0f));

        // --------------------------- Copa (hojas) ----------------------------
        int leafTotal = lightweight ? 11000 : 44000;   // muchas hojas finas
        int perCluster = leafTotal / static_cast<int>(canopyCenters.size());
        for (size_t i = 0; i < canopyCenters.size(); ++i) {
            float rad = (i + 1 == canopyCenters.size()) ? 1.5f : 1.0f;
            GeometryUtils::generateSphere(particles, canopyCenters[i], rad,
                                          perCluster, TYPE_LEAF, leafColor, leafPSize);
        }

        // ----------------------- Tiempos de ignición ------------------------
        // Hojas: encendido ALEATORIO repartido por toda la copa (distribuido).
        // Madera: más tarde y de ARRIBA hacia abajo (tras las hojas).
        for (auto& p : particles) {
            if (p.type == TYPE_GROUND) {
                p.velocity.w = 100000.0f;
            } else if (p.type == TYPE_LEAF) {
                // Variación de tono de verde para que el follaje se vea natural.
                float v = 0.78f + r01(rng) * 0.5f;
                p.color.r *= v; p.color.g *= v; p.color.b *= v;
                glm::vec3 pos(p.position);
                float spatial = 0.5f * std::sin(pos.x * 3.1f + pos.z * 2.3f + pos.y * 1.7f);
                p.velocity.w = 0.4f + r01(rng) * 3.4f + std::max(0.0f, spatial);
            } else if (p.type == TYPE_WOOD) {
                float y = p.position.y;
                p.velocity.w = 4.0f + (trunkH - y) * 1.4f + r01(rng) * 1.0f;
            }
        }

        // ---- Emisores con VENTANA TEMPORAL (copa primero, tronco después) ----
        auto addEmitter = [&](glm::vec3 posE, EmitterShape shape, float radE,
                              float rate, float life, float spd, float sz,
                              float tStart, float tEnd, float fIn, float fOut) {
            EmitterConfig e;
            e.position = posE;
            e.shape = shape;
            e.radius = radE;
            e.direction = glm::vec3(0.0f, 1.0f, 0.0f);
            e.emitRate = lightweight ? rate * 0.4f : rate;
            e.particleLife = life;
            e.lifeVariance = 0.5f;
            e.initialSpeed = spd;
            e.speedVariance = 0.45f;
            e.temperature = 1.0f;
            e.particleSize = sz;
            e.startTime = tStart;
            e.endTime = tEnd;
            e.fadeIn = fIn;
            e.fadeOut = fOut;
            emitters.push_back(e);
        };

        // COPA: un emisor de fuego por racimo de hojas -> LLAMAS REALES y
        // DISTRIBUIDAS por todo el follaje, activas mientras hay hojas (~1-12s).
        for (size_t i = 0; i < canopyCenters.size(); ++i) {
            float rad = (i + 1 == canopyCenters.size()) ? 1.35f : 0.9f;
            addEmitter(canopyCenters[i], EmitterShape::SPHERE, rad,
                       9000.0f, 0.95f, 0.7f, lightweight ? 0.05f : 0.034f,
                       0.8f, 12.0f, 1.8f, 4.0f);
        }

        // TRONCO/RAMAS: arden DESPUÉS de las hojas (ventana más tardía).
        addEmitter(glm::vec3(0.0f, 0.25f, 0.0f), EmitterShape::DISK, 0.22f,
                   15000.0f, 1.1f, 1.1f, lightweight ? 0.05f : 0.038f,
                   5.0f, 22.0f, 2.5f, 6.0f);
        addEmitter(glm::vec3(0.0f, 1.35f, 0.0f), EmitterShape::DISK, 0.18f,
                   10000.0f, 1.2f, 1.3f, lightweight ? 0.045f : 0.034f,
                   5.5f, 22.0f, 2.5f, 6.0f);

        // --------------- Curl noise: turbulencia de las llamas --------------
        curlParams.frequency = 1.7f;
        curlParams.amplitude = 2.2f;
        curlParams.octaves = 4;
        curlParams.lacunarity = 2.0f;
        curlParams.persistence = 0.5f;
        curlParams.timeScale = 1.15f;

        // --------------------------- Cámara ---------------------------------
        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        glm::vec3 target(0.0f, 2.1f, 0.0f);
        cameraPath.keyframes.push_back({0.0f,  glm::vec3(0.0f, 2.4f, 6.5f), target});
        cameraPath.keyframes.push_back({8.0f,  glm::vec3(4.8f, 2.0f, 4.2f), target});
        cameraPath.keyframes.push_back({16.0f, glm::vec3(4.6f, 1.6f, -3.6f), glm::vec3(0.0f, 1.5f, 0.0f)});
        cameraPath.keyframes.push_back({24.0f, glm::vec3(-1.0f, 2.2f, 6.2f), target});
    }
};

} // namespace

std::unique_ptr<Scene> createTreeScene() {
    return std::make_unique<TreeScene>();
}
