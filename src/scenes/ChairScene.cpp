#include "Scene.h"
#include <random>
#include <cmath>
#include <algorithm>

// Escena de calidad: silla de madera con papeles encima que arde y se desmorona.
// Demuestra interacción con materiales: el papel prende rápido, la madera lento,
// y la estructura colapsa (desintegración con gravedad) mientras se consume.
class ChairScene : public Scene {
public:
    std::string getName() const override { return "Chair"; }

    float getDuration() const override { return 22.0f; }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(0.10f, 0.02f, 0.02f); // rojo oscuro tipo referencia
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight = false) override {

        particles.clear();
        emitters.clear();

        glm::vec4 woodColor(0.42f, 0.26f, 0.12f, 1.0f);
        glm::vec4 paperColor(0.92f, 0.86f, 0.72f, 1.0f);
        float pSize = lightweight ? 0.018f : 0.012f;

        // Densidad de las cajas (más = más sólido)
        int d = lightweight ? 1 : 2;

        // --- Geometría de la silla (vista trasera, como la referencia) ---
        float seatY = 0.45f, hx = 0.22f, hz = 0.22f;

        // Asiento
        GeometryUtils::generateBox(particles, glm::vec3(0.0f, seatY, 0.0f),
                                   glm::vec3(0.50f, 0.06f, 0.50f), 40*d, 5*d, 40*d,
                                   TYPE_WOOD, woodColor, pSize);
        // 4 patas
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                GeometryUtils::generateBox(particles,
                    glm::vec3(sx*hx, seatY*0.5f, sz*hz),
                    glm::vec3(0.06f, seatY, 0.06f), 5*d, 36*d, 5*d,
                    TYPE_WOOD, woodColor, pSize);
        // 2 postes traseros
        for (int sx = -1; sx <= 1; sx += 2)
            GeometryUtils::generateBox(particles,
                glm::vec3(sx*hx, seatY + 0.35f, -hz),
                glm::vec3(0.06f, 0.70f, 0.06f), 5*d, 56*d, 5*d,
                TYPE_WOOD, woodColor, pSize);
        // 3 listones del respaldo
        for (int k = 0; k < 3; ++k)
            GeometryUtils::generateBox(particles,
                glm::vec3(0.0f, seatY + 0.25f + k*0.20f, -hz),
                glm::vec3(0.46f, 0.06f, 0.05f), 40*d, 5*d, 4*d,
                TYPE_WOOD, woodColor, pSize);

        // --- Hojas de papel sobre el asiento ---
        int paperU = lightweight ? 120 : 220;
        int paperV = lightweight ? 120 : 220;
        for (int k = 0; k < 3; ++k) {
            glm::vec3 c(((k - 1) * 0.06f), seatY + 0.035f + k * 0.006f, (k - 1) * 0.05f);
            GeometryUtils::generatePlane(particles, c,
                glm::vec3(1,0,0), 0.34f, paperU,
                glm::vec3(0,0,1), 0.34f, paperV,
                TYPE_PAPER, paperColor, lightweight ? 0.006f : 0.004f);
        }

        // --- Pool de partículas muertas (fuego/humo/brasas/ceniza) ---
        size_t deadPool = lightweight ? 200000 : 700000;
        for (size_t i = 0; i < deadPool; ++i) {
            Particle p{}; p.type = TYPE_DEAD; particles.push_back(p);
        }

        // --- Coreografía de quemado: empieza en el papel del asiento y se propaga ---
        glm::vec3 ignition(0.06f, seatY + 0.05f, 0.04f); // ignición off-center -> frente diagonal
        std::mt19937 rng(7);
        std::uniform_real_distribution<float> jitter(-0.35f, 0.35f); // más irregular = borde rasgado
        for (auto& p : particles) {
            glm::vec3 pos = glm::vec3(p.position);
            float dist = glm::distance(pos, ignition);
            if (p.type == TYPE_PAPER) {
                // Frente de quemado LENTO y visible que barre la hoja (no flamazo).
                p.velocity.w = std::max(0.0f, dist / 0.085f + jitter(rng));
            } else if (p.type == TYPE_WOOD) {
                // La madera tarda: el fuego baja del asiento hacia patas/respaldo.
                // Cuanto más abajo, más tarde (calor sube, la base cae al final).
                float heightDelay = (seatY - pos.y) * 2.0f; // patas arden después
                p.velocity.w = std::max(0.0f, 3.0f + dist / 0.10f + heightDelay + jitter(rng));
            }
        }

        // --- Emisor de llamas CONTENIDO sobre el papel ---
        // Modesto a propósito: el protagonista es el frente de carbonización del
        // papel, no un flamazo. Estas llamas sólo lamen desde la zona que arde.
        EmitterConfig fire;
        fire.position = glm::vec3(0.0f, seatY + 0.06f, 0.0f);
        fire.direction = glm::vec3(0.0f, 1.0f, 0.0f);
        fire.shape = EmitterShape::RECTANGLE;
        fire.width = 0.34f; fire.height = 0.34f;    // cubre el papel
        fire.emitRate = lightweight ? 3500.0f : 9000.0f; // bajo -> no tapa el papel
        fire.particleLife = 0.7f;                   // vida corta -> llama baja y contenida
        fire.initialSpeed = 0.55f;                  // sube despacio
        fire.speedVariance = 0.25f;
        fire.particleSize = 0.045f;                 // se funden sin saturar
        fire.temperature = 1.0f;
        emitters.push_back(fire);

        // Curl suave: turbulencia de llama sin explosión.
        curlParams.frequency = 1.4f;
        curlParams.amplitude = 1.1f;
        curlParams.octaves = 3;
        curlParams.lacunarity = 2.0f;
        curlParams.persistence = 0.5f;
        curlParams.timeScale = 1.3f;
        curlParams.epsilon = 0.01f;

        // Cámara: CERCA y en picado oblicuo sobre el asiento, para ver el frente
        // de quemado barrer la hoja y las brasas subir (estilo referencia ffuego).
        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        glm::vec3 target(0.0f, 0.50f, 0.0f); // mira el papel sobre el asiento
        cameraPath.keyframes.push_back({0.0f,  glm::vec3(0.10f, 0.98f, 1.05f), target});
        cameraPath.keyframes.push_back({11.0f, glm::vec3(0.30f, 0.90f, 0.92f), target});
        cameraPath.keyframes.push_back({22.0f, glm::vec3(-0.28f, 0.95f, 1.00f), target});
    }
};
