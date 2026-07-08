#include "Test6Scene.h"
#include <iostream>
#include <random>

// =============================================================================
// Test6Scene - Barco de origami quemándose desde la proa
//
// El barco se construye con 6 planos en distintos ángulos:
//   - Base plana (fondo del barco)
//   - Casco izquierdo y derecho (inclinados ~35° hacia afuera)
//   - Vela central (plano vertical)
//   - Proa y popa (triángulos en los extremos, simulados con planos estrechos)
//
// El fuego arranca desde la punta de la proa y se propaga hacia la popa.
// =============================================================================

class Test6SceneImpl : public Scene {
public:
    std::string getName() const override {
        return "Barco de origami quemándose";
    }

    float getDuration() const override {
        return 30.0f;
    }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(1.0f, 1.0f, 1.0f);
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {

        emitters.clear();

        // Color papel crema
        glm::vec4 paperColor(0.97f, 0.94f, 0.86f, 1.0f);

        // Resolución de cada panel según calidad
        int hiU = lightweight ? 200 : 300;
        int hiV = lightweight ? 100 : 150;
        int loU = lightweight ? 100 : 150;
        int loV = lightweight ? 60  : 90;
        float pSize = lightweight ? 0.0052f : 0.0044f;

        // -----------------------------------------------------------------
        // BASE DEL BARCO — plano horizontal en el centro
        // El barco mide ~1.2 en Z (largo) y ~0.5 en X (ancho)
        // -----------------------------------------------------------------
        glm::vec3 baseCenter(0.0f, 0.0f, 0.0f);
        GeometryUtils::generatePlane(
            particles, baseCenter,
            glm::vec3(1.0f, 0.0f, 0.0f), 0.50f, hiU,
            glm::vec3(0.0f, 0.0f, 1.0f), 1.20f, hiV,
            TYPE_PAPER, paperColor, pSize
        );

        // -----------------------------------------------------------------
        // CASCO IZQUIERDO — inclinado ~35° hacia la izquierda
        // Pivota en el borde izquierdo de la base (x = -0.25)
        // -----------------------------------------------------------------
        // cos(35°)≈0.819, sin(35°)≈0.574
        glm::vec3 cascoIzqCenter(-0.25f + (-0.20f * 0.819f), 0.20f * 0.574f, 0.0f);
        glm::vec3 cascoIzqN( 0.819f, 0.574f, 0.0f); // normal apuntando arriba-derecha
        // dirección U = largo del barco (Z), dirección V = perpendicular al plano inclinado
        glm::vec3 cascoV(-0.574f, 0.819f, 0.0f);    // perpendicular en el plano XY inclinado
        GeometryUtils::generatePlane(
            particles, cascoIzqCenter,
            glm::vec3(0.0f, 0.0f, 1.0f), 1.20f, hiV,
            cascoV, 0.40f, loV,
            TYPE_PAPER, paperColor, pSize
        );

        // -----------------------------------------------------------------
        // CASCO DERECHO — espejo del izquierdo
        // -----------------------------------------------------------------
        glm::vec3 cascoDeCenter( 0.25f + ( 0.20f * 0.819f), 0.20f * 0.574f, 0.0f);
        glm::vec3 coscoDerV( 0.574f, 0.819f, 0.0f);
        GeometryUtils::generatePlane(
            particles, cascoDeCenter,
            glm::vec3(0.0f, 0.0f, 1.0f), 1.20f, hiV,
            coscoDerV, 0.40f, loV,
            TYPE_PAPER, paperColor, pSize
        );

        // -----------------------------------------------------------------
        // VELA CENTRAL — plano vertical a lo largo del barco
        // -----------------------------------------------------------------
        glm::vec3 velaCenter(0.0f, 0.35f, 0.0f);
        GeometryUtils::generatePlane(
            particles, velaCenter,
            glm::vec3(0.0f, 0.0f, 1.0f), 0.70f, loV,
            glm::vec3(0.0f, 1.0f, 0.0f), 0.30f, loU,
            TYPE_PAPER, paperColor, pSize
        );

        // -----------------------------------------------------------------
        // PROA (frente, z = -0.60) — plano diagonal que cierra el frente
        // Inclinado ~45° en el plano XZ
        // -----------------------------------------------------------------
        glm::vec3 proaCenter(0.0f, 0.15f, -0.55f);
        GeometryUtils::generatePlane(
            particles, proaCenter,
            glm::vec3(1.0f, 0.0f, 0.0f), 0.50f, loU,
            glm::vec3(0.0f, 1.0f, 0.28f), 0.35f, loV,
            TYPE_PAPER, paperColor, pSize
        );

        // -----------------------------------------------------------------
        // POPA (atrás, z = +0.60) — cierra la parte trasera
        // -----------------------------------------------------------------
        glm::vec3 popaCenter(0.0f, 0.15f, 0.55f);
        GeometryUtils::generatePlane(
            particles, popaCenter,
            glm::vec3(1.0f, 0.0f, 0.0f), 0.50f, loU,
            glm::vec3(0.0f, 1.0f, -0.28f), 0.35f, loV,
            TYPE_PAPER, paperColor, pSize
        );

        // -----------------------------------------------------------------
        // PUNTO DE IGNICIÓN — punta de la proa
        // -----------------------------------------------------------------
        glm::vec3 ignitionPoint(0.0f, 0.05f, -0.60f);
        float baseBurnSpeed = 0.090f;

        // Emisor de llama en la proa
        EmitterConfig flameFront;
        flameFront.position    = ignitionPoint + glm::vec3(0.0f, 0.05f, -0.02f);
        flameFront.shape       = EmitterShape::DISK;
        flameFront.radius      = 0.04f;
        flameFront.direction   = glm::vec3(0.0f, 1.0f, 0.1f);
        flameFront.emitRate    = lightweight ? 1600.0f : 3000.0f;
        flameFront.particleLife    = 0.80f;
        flameFront.lifeVariance    = 0.28f;
        flameFront.initialSpeed    = 0.70f;
        flameFront.speedVariance   = 0.32f;
        flameFront.temperature     = 1.0f;
        flameFront.particleSize    = lightweight ? 0.020f : 0.016f;
        emitters.push_back(flameFront);

        // -----------------------------------------------------------------
        // BURN TIMES — propagación desde la proa con irregularidad
        // -----------------------------------------------------------------
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        for (auto& p : particles) {
            if (p.type == TYPE_PAPER) {
                glm::vec3 pos = glm::vec3(p.position);
                float d = glm::distance(pos, ignitionPoint);

                // Ruido para simular variaciones de grosor del papel doblado
                float noise = sin(pos.x * 12.0f) * cos(pos.z * 10.0f) * 0.70f
                            + sin(pos.x * 25.0f + 0.8f) * cos(pos.z * 22.0f) * 0.40f
                            + dist(rng) * 0.30f;

                float thermalAcceleration = std::pow(d, 0.88f);
                // Algunas partículas en los pliegues son más resistentes
                float fragility = (dist(rng) > 0.90f) ? 0.40f : 1.0f;

                float burnTime = (thermalAcceleration / baseBurnSpeed) + noise * 1.4f * fragility;
                p.velocity.w = std::max(0.0f, burnTime);
            }
        }

        // -----------------------------------------------------------------
        // CURL NOISE — turbulencia de las llamas
        // -----------------------------------------------------------------
        curlParams.frequency   = 3.2f;
        curlParams.amplitude   = 1.0f;
        curlParams.octaves     = 3;
        curlParams.lacunarity  = 2.0f;
        curlParams.persistence = 0.5f;
        curlParams.timeScale   = 1.6f;
        curlParams.epsilon     = 0.01f;

        // -----------------------------------------------------------------
        // CÁMARA — ligera elevación, vista desde el frente-lateral
        // -----------------------------------------------------------------
        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        // Arranca desde el lateral-frente y hace un giro lento hacia la popa
        cameraPath.keyframes.push_back({  0.0f, glm::vec3( 1.20f, 0.80f, -1.0f), glm::vec3(0.0f, 0.15f, 0.0f)});
        cameraPath.keyframes.push_back({ 15.0f, glm::vec3( 0.20f, 1.00f,  1.6f), glm::vec3(0.0f, 0.15f, 0.0f)});
        cameraPath.keyframes.push_back({ 30.0f, glm::vec3(-1.20f, 0.80f, -0.8f), glm::vec3(0.0f, 0.10f, 0.0f)});
    }
};

std::unique_ptr<Scene> createTest6Scene() {
    return std::make_unique<Test6SceneImpl>();
}
