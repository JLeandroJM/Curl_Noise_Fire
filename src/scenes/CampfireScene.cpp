// =============================================================================
// CampfireScene.cpp - Fogata de leña (escena "hero")
//
// Filosofía: el FUEGO NACE DE LA MADERA que arde, no de un emisor constante.
// Asi se obtiene de forma natural lo que pide una fogata real:
//   - múltiples fuentes distribuidas (cada leño que arde es una fuente)
//   - crece, fluctúa (encendido escalonado en oleadas) y DECAE cuando la
//     madera se consume (el fuego no es eterno)
//   - se ve la madera (llamas mas bajas, leños desordenados y finos)
//
// La conversión madera->fuego/humo/brasa vive en particle_update.comp
// (rama TYPE_WOOD). Aqui solo construimos geometría desordenada y definimos
// CUÁNDO enciende cada leño (velocity.w = burnStartTime).
// =============================================================================

#include "CampfireScene.h"
#include <cmath>
#include <random>
#include <vector>

namespace {

class CampfireScene : public Scene {
public:
    std::string getName() const override { return "Fogata (Madera)"; }

    float getDuration() const override { return 24.0f; }

    glm::vec3 getBackgroundColor() const override {
        // Entorno oscuro y cálido: el fuego es la fuente de luz.
        return glm::vec3(0.02f, 0.016f, 0.012f);
    }

    // Intensidad del fuego en el tiempo: prende (0->1), arde al MÁXIMO mientras
    // la madera está sana (~1.5-9s), y se APAGA al carbonizarse (9->17s -> 0).
    float getEmitScale(float t) const override {
        float s;
        if (t < 1.5f)       s = t / 1.5f;                 // prende
        else if (t < 9.0f)  s = 1.0f;                     // máximo (leña sana)
        else if (t < 17.0f) s = 1.0f - (t - 9.0f) / 8.0f; // se apaga al carbonizarse
        else                s = 0.0f;                     // solo carbón + humo
        if (s < 0.0f) s = 0.0f;
        if (s > 1.0f) s = 1.0f;
        // Parpadeo natural de la llama.
        s *= 0.86f + 0.14f * std::sin(t * 6.1f) * std::cos(t * 2.3f);
        return s < 0.0f ? 0.0f : s;
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {

        emitters.clear();
        // No limpiamos 'particles': SceneManager ya pre-llena el pool de muertas.

        std::mt19937 rng(2024);
        std::uniform_real_distribution<float> r01(0.0f, 1.0f);
        auto rnd = [&](float a, float b) { return a + (b - a) * r01(rng); };

        // Densidad por unidad de longitud (para que todos los leños se vean igual de finos).
        const int   radial   = lightweight ? 10 : 24;
        const float partPerM = lightweight ? 60.0f : 150.0f;   // anillos por metro
        const float logPSize = lightweight ? 0.019f : 0.0095f;

        // --------- Asigna burnStart a las partículas recién agregadas ---------
        // delay: cuándo empieza la oleada. Las partes bajas encienden antes.
        auto markBurn = [&](size_t from, float delay, float spread) {
            for (size_t k = from; k < particles.size(); ++k) {
                Particle& p = particles[k];
                float y = p.position.y;
                p.velocity.w = delay + y * 0.7f + r01(rng) * spread;
            }
        };

        // ------------------------------ Suelo --------------------------------
        glm::vec4 groundColor(0.085f, 0.06f, 0.045f, 1.0f);
        size_t gFrom = particles.size();
        int gc = lightweight ? 70 : 130;
        GeometryUtils::generatePlane(
            particles, glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1, 0, 0), 4.4f, gc,
            glm::vec3(0, 0, 1), 4.4f, gc,
            TYPE_GROUND, groundColor, lightweight ? 0.04f : 0.022f);
        for (size_t k = gFrom; k < particles.size(); ++k) {
            particles[k].velocity.w = 100000.0f;   // el suelo no arde
        }

        // Helper para crear un leño entre 'base' y 'apex' con color marrón variado.
        auto addLog = [&](glm::vec3 base, glm::vec3 apex, float radius,
                          float delay, float spread) {
            glm::vec3 axis = apex - base;
            float h = glm::length(axis);
            if (h < 0.02f) return;
            axis = glm::normalize(axis);
            int hc = std::max(6, static_cast<int>(h * partPerM));
            // Marrón de madera con variación (vetas/tono) por leño.
            float tone = rnd(0.85f, 1.15f);
            glm::vec4 logColor(0.30f * tone, 0.155f * tone, 0.065f * tone, 1.0f);
            size_t from = particles.size();
            GeometryUtils::generateCylinder(particles, base, axis, radius, h,
                                            radial, hc, TYPE_WOOD, logColor, logPSize);
            markBurn(from, delay, spread);
        };

        // ------------------- Astillas/kindling al centro ---------------------
        // Encienden primero -> flama inicial.
        for (int i = 0; i < (lightweight ? 4 : 8); ++i) {
            float a = rnd(0.0f, 6.2831853f);
            float r = rnd(0.0f, 0.18f);
            glm::vec3 b(std::cos(a) * r, 0.01f, std::sin(a) * r);
            glm::vec3 t(std::cos(a) * r * 0.3f + rnd(-0.05f, 0.05f),
                        rnd(0.18f, 0.34f),
                        std::sin(a) * r * 0.3f + rnd(-0.05f, 0.05f));
            addLog(b, t, rnd(0.018f, 0.030f), rnd(0.15f, 0.6f), 0.4f);
        }

        // -------------------- Leños en cono (desordenado) -------------------
        // Oleadas escalonadas: arden, bajan, vuelven a arder.
        const int numLogs = lightweight ? 5 : 8;
        float waveDelays[8] = { 0.6f, 0.9f, 2.6f, 3.1f, 5.6f, 6.4f, 8.8f, 9.6f };
        for (int i = 0; i < numLogs; ++i) {
            float a = (static_cast<float>(i) / numLogs) * 6.2831853f + rnd(-0.25f, 0.25f);
            float ring = rnd(0.42f, 0.62f);
            glm::vec3 base(std::cos(a) * ring, rnd(0.0f, 0.04f), std::sin(a) * ring);
            // ápice cerca del centro pero desordenado (no perfecto).
            glm::vec3 apex(rnd(-0.14f, 0.14f), rnd(0.80f, 1.15f), rnd(-0.14f, 0.14f));
            float radius = rnd(0.045f, 0.085f);
            float delay = waveDelays[i % 8] + rnd(-0.3f, 0.3f);
            addLog(base, apex, radius, delay, 1.4f);
        }

        // ----------------------- Troncos caídos -----------------------------
        // Acostados cruzando la pila -> rompen la simetría, arden mas tarde.
        for (int i = 0; i < (lightweight ? 1 : 3); ++i) {
            float a = rnd(0.0f, 6.2831853f);
            float len = rnd(1.1f, 1.7f);
            float ylie = rnd(0.06f, 0.16f);
            glm::vec3 mid(rnd(-0.15f, 0.15f), ylie, rnd(-0.15f, 0.15f));
            glm::vec3 dirh(std::cos(a), rnd(-0.05f, 0.12f), std::sin(a));
            dirh = glm::normalize(dirh);
            glm::vec3 base = mid - dirh * (len * 0.5f);
            glm::vec3 apex = mid + dirh * (len * 0.5f);
            base.y = std::max(0.03f, base.y);
            addLog(base, apex, rnd(0.055f, 0.085f), rnd(5.0f, 8.0f), 2.0f);
        }

        // ---------- Emisores de fuego (cuerpo de llama) ----------
        // Su intensidad la modula getEmitScale() en el tiempo: 0 -> máximo
        // (mientras la madera está buena) -> 0 (cuando se carboniza). Asi el
        // fuego arde fuerte con leña sana y se apaga al volverse carbón.
        EmitterConfig base;
        base.position = glm::vec3(0.0f, 0.16f, 0.0f);
        base.shape = EmitterShape::DISK;
        base.radius = 0.40f;
        base.direction = glm::vec3(0.0f, 1.0f, 0.0f);
        base.emitRate = lightweight ? 12000.0f : 32000.0f;
        base.particleLife = 1.0f;
        base.lifeVariance = 0.5f;
        base.initialSpeed = 1.05f;
        base.speedVariance = 0.5f;
        base.temperature = 1.0f;
        base.particleSize = lightweight ? 0.055f : 0.040f;
        emitters.push_back(base);

        EmitterConfig core;
        core.position = glm::vec3(0.0f, 0.28f, 0.0f);
        core.shape = EmitterShape::DISK;
        core.radius = 0.15f;
        core.direction = glm::vec3(0.0f, 1.0f, 0.0f);
        core.emitRate = lightweight ? 3500.0f : 9000.0f;
        core.particleLife = 1.3f;
        core.lifeVariance = 0.5f;
        core.initialSpeed = 1.7f;
        core.speedVariance = 0.5f;
        core.temperature = 1.0f;
        core.particleSize = lightweight ? 0.05f : 0.034f;
        emitters.push_back(core);

        // --------------- Curl noise: turbulencia de las llamas --------------
        curlParams.frequency = 2.0f;
        curlParams.amplitude = 2.3f;
        curlParams.octaves = 4;
        curlParams.lacunarity = 2.0f;
        curlParams.persistence = 0.5f;
        curlParams.timeScale = 1.2f;

        // ------- Cámara: órbita lenta y baja para ver leños y llamas --------
        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        glm::vec3 target(0.0f, 0.45f, 0.0f);
        cameraPath.keyframes.push_back({0.0f,  glm::vec3(-1.2f, 0.85f, 3.1f), target});
        cameraPath.keyframes.push_back({8.0f,  glm::vec3( 1.9f, 1.05f, 2.7f), target});
        cameraPath.keyframes.push_back({16.0f, glm::vec3( 2.6f, 0.95f, -1.4f), target});
        cameraPath.keyframes.push_back({24.0f, glm::vec3(-0.6f, 1.25f, 3.2f), target});
    }
};

} // namespace

std::unique_ptr<Scene> createCampfireScene() {
    return std::make_unique<CampfireScene>();
}
