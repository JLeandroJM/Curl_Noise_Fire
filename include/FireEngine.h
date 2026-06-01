// =============================================================================
// FireEngine.h - API de host del motor CUDA de fuego.
// Simulación de partículas (emit + update con curl noise) + rasterizado por
// splatting + bloom/tonemap, todo en GPU. Salida: RGBA8 premultiplicado.
// =============================================================================
#pragma once

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include "Particle.h"   // struct Particle, EmitterConfig, CurlNoiseParams (host)

struct RenderParams {
    float bloomThreshold  = 1.0f;   // brillo mínimo para el bloom
    float bloomIntensity  = 0.8f;   // cuánto se suma el bloom
    int   bloomIterations = 5;      // pasadas de blur separable
    float exposure        = 1.3f;   // exposición antes del tonemap
    float smokeDensity    = 1.6f;   // escala de opacidad del humo
    float particleGlow    = 1.2f;   // multiplicador de brillo emisivo (fuego/chispa)
    bool  showGeometry    = false;  // dibujar materiales estáticos (preview, no para composición)
    bool  tiled           = true;   // rasterizador tiled (recomendado). false = splat simple (fallback)
    int   tileSize        = 16;     // 16x16 = 256 hilos/bloque (no cambiar sin ajustar shared mem)
};

class FireEngine {
public:
    FireEngine();
    ~FireEngine();

    // Sube el pool inicial de partículas y los emisores a la GPU.
    void init(int width, int height,
              const std::vector<Particle>& initialParticles,
              const std::vector<EmitterConfig>& emitters,
              const CurlNoiseParams& curl,
              const glm::vec3& windDir, float windStrength);

    // Un paso de simulación (update + emit). dt fijo recomendado para video.
    void update(float dt, float simTime);

    // Renderiza el estado actual. viewProj = glm::mat4 (column-major, 16 floats).
    // proj11 = proj[1][1] (= 1/tan(fov/2)), usado para el tamaño en píxeles.
    // Escribe RGBA8 premultiplicado, origen arriba-izquierda, en `outRGBA` (w*h*4).
    void render(const glm::mat4& viewProj, float proj11,
                const RenderParams& rp, std::vector<uint8_t>& outRGBA);

    void shutdown();

    int width()  const { return width_; }
    int height() const { return height_; }
    int numParticles() const { return maxParticles_; }

private:
    int width_  = 0;
    int height_ = 0;
    int maxParticles_ = 0;
    int numEmitters_  = 0;
    unsigned int frame_ = 0;

    // Punteros device (definidos en el .cu).
    void* d_particles_ = nullptr;   // GpuParticle*
    void* d_emitters_  = nullptr;   // GpuEmitter*
    void* d_emissive_  = nullptr;   // float4*  (acumulación aditiva emisiva)
    void* d_smoke_     = nullptr;   // float4*  (humo: Σ c*a*w en xyz, Σ a*w en w)
    void* d_smokeW_    = nullptr;   // float*   (humo: Σ w)
    void* d_hdr_       = nullptr;   // float4*  (color HDR premult resuelto + alpha)
    void* d_bloomA_    = nullptr;   // float4*  ping
    void* d_bloomB_    = nullptr;   // float4*  pong
    void* d_out_       = nullptr;   // uchar4*  (RGBA8 final)
    uint8_t* h_pinned_ = nullptr;   // staging pinned host

    // --- Rasterizador tiled (2DGS-style) ---
    void* d_visMu_      = nullptr;  // float2*  centro en pantalla (px)
    void* d_visSigma_   = nullptr;  // float*   sigma de la gaussiana (px)
    void* d_visDepth_   = nullptr;  // float*   profundidad (clip.w) para ordenar
    void* d_visOpacity_ = nullptr;  // float*   opacidad base
    void* d_visColor_   = nullptr;  // float4*  color (rgb)
    void* d_tileBox_    = nullptr;  // int4*    (tx0,ty0,tx1,ty1) tiles cubiertos
    void* d_tileCount_  = nullptr;  // int*     nº de tiles por partícula
    void* d_tileOffset_ = nullptr;  // int*     prefix-sum (offset en pares)
    void* d_keys_       = nullptr;  // uint64*  claves (tile<<32 | depth)
    void* d_vals_       = nullptr;  // int*     id de partícula por par
    void* d_ranges_     = nullptr;  // int2*    rango [start,end) por tile
    size_t pairCapacity_ = 0;       // capacidad asignada de keys/vals
    int tilesX_ = 0, tilesY_ = 0, numTiles_ = 0;

    // Estado de simulación.
    glm::vec3 windDir_ = glm::vec3(0.0f);
    float windStrength_ = 0.0f;
    glm::vec3 gravity_ = glm::vec3(0.0f, -9.81f, 0.0f);
    float buoyancy_ = 3.0f;
    // Curl params se copian a un blob simple en el .cu.
    float curl_[7] = {1.5f, 2.0f, 4.0f, 2.0f, 0.5f, 0.8f, 0.01f};
};
