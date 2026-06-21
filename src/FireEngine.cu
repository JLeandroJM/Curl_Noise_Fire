// =============================================================================
// FireEngine.cu - Motor CUDA: simulación de partículas + rasterizado por
// splatting + bloom/tonemap. Todo headless (sin OpenGL).
// =============================================================================
#include "FireEngine.h"
#include "gpu_types.cuh"
#include "CurlNoise.cuh"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <thrust/device_ptr.h>
#include <thrust/scan.h>
#include <thrust/sort.h>
#include <thrust/execution_policy.h>

// ---------------------------------------------------------------------------
// Manejo de errores
// ---------------------------------------------------------------------------
#define CUDA_CHECK(call)                                                      \
    do {                                                                      \
        cudaError_t _e = (call);                                              \
        if (_e != cudaSuccess) {                                              \
            std::fprintf(stderr, "CUDA error %s:%d: %s\n", __FILE__, __LINE__,\
                         cudaGetErrorString(_e));                             \
            std::exit(EXIT_FAILURE);                                          \
        }                                                                     \
    } while (0)

static const int BLOCK = 256;
static const int kTileSize = 16; // 16x16 = 256 hilos/bloque en el blend tiled

// Matriz 4x4 column-major (glm) pasada por valor a los kernels.
struct Mat4 { float m[16]; };

__device__ inline float4 mulMat4Point(const Mat4& M, float3 p) {
    // M column-major: elemento (fila r, col c) = m[c*4 + r]
    float x = M.m[0]*p.x + M.m[4]*p.y + M.m[8]*p.z  + M.m[12];
    float y = M.m[1]*p.x + M.m[5]*p.y + M.m[9]*p.z  + M.m[13];
    float z = M.m[2]*p.x + M.m[6]*p.y + M.m[10]*p.z + M.m[14];
    float w = M.m[3]*p.x + M.m[7]*p.y + M.m[11]*p.z + M.m[15];
    return make_float4(x, y, z, w);
}

// ---------------------------------------------------------------------------
// Gradiente de color del fuego por temperatura (igual que particle_update.comp)
// ---------------------------------------------------------------------------
__device__ inline float4 getFireColor(float temp) {
    // Paleta cálida: blanco-amarillento SOLO en el núcleo más caliente; el grueso
    // de la llama es naranja->rojo->rojo oscuro (menos blowout, más realista).
    if (temp > 0.92f) return mix4(make_float4(1,0.95f,0.55f,1), make_float4(1,1,0.9f,1), (temp-0.92f)/0.08f);
    if (temp > 0.6f)  return mix4(make_float4(1,0.45f,0.05f,1), make_float4(1,0.95f,0.55f,1), (temp-0.6f)/0.32f);
    if (temp > 0.3f)  return mix4(make_float4(0.85f,0.1f,0.0f,1), make_float4(1,0.45f,0.05f,1), (temp-0.3f)/0.3f);
    if (temp > 0.15f) return mix4(make_float4(0.4f,0.02f,0.0f,1), make_float4(0.85f,0.1f,0.0f,1), (temp-0.15f)/0.15f);
    return mix4(make_float4(0,0,0,1), make_float4(0.4f,0.02f,0.0f,1), temp/0.15f);
}

// ===========================================================================
// SIMULACIÓN
// ===========================================================================
__global__ void updateKernel(GpuParticle* P, int n, float dt, float t,
                             GpuCurlParams cp, float3 gravity, float buoyancy,
                             float3 windDir, float windStrength) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    GpuParticle p = P[idx];
    if (p.type == G_TYPE_DEAD) return;

    p.lifetime -= dt;
    if (p.lifetime <= 0.0f && p.type < 10u) {
        p.type = G_TYPE_DEAD;
        P[idx] = p;
        return;
    }

    float3 pos = make_float3(p.position.x, p.position.y, p.position.z);
    float3 vel = make_float3(p.velocity.x, p.velocity.y, p.velocity.z);
    float age_ratio = clampf(1.0f - (p.lifetime / p.maxLifetime), 0.0f, 1.0f);
    float3 force = windDir * windStrength;

    if (p.type == G_TYPE_FIRE) {
        p.temperature -= dt * 0.85f; // enfría más rápido -> llama corta que enrojece arriba
        p.temperature = fmaxf(p.temperature, 0.0f);
        float3 buoy = make_float3(0.0f, 1.0f, 0.0f) * (buoyancy * p.temperature);
        float3 curl = curlNoise(pos, t, cp);
        // Curl reducido (antes *5 = demasiado explosivo). Más flotabilidad, menos
        // dispersión lateral -> fuego concentrado que sube en columna/lengua.
        vel += (buoy + curl * 1.8f + force) * dt;
        vel.x *= 0.96f; vel.z *= 0.96f; // amortigua dispersión horizontal (concentra)
        pos += vel * dt;
        p.color = getFireColor(p.temperature);
        p.color.w *= (1.0f - age_ratio);
    }
    else if (p.type == G_TYPE_EMBER) {
        vel += (gravity * 0.2f + force * 0.5f) * dt;
        pos += vel * dt;
        float pulse = sinf(t * 10.0f + (float)idx) * 0.5f + 0.5f;
        p.color = make_float4(1.0f, 0.2f * pulse, 0.0f, 1.0f);
        p.color.w *= (1.0f - age_ratio);
    }
    else if (p.type == G_TYPE_SMOKE) {
        vel += (make_float3(0.0f, 0.5f, 0.0f) + force) * dt;
        pos += vel * dt;
        p.position.w += dt * 2.0f;
        p.color = make_float4(0.5f, 0.5f, 0.5f, 1.0f);
        p.color.w *= fmaxf(0.0f, 1.0f - age_ratio * 1.5f);
    }
    else if (p.type == G_TYPE_ASH) {
        float3 drift = make_float3(snoise3(make_float3(pos.y * 10.0f, t, 0.0f)),
                                   0.0f,
                                   snoise3(make_float3(0.0f, t, pos.y * 10.0f)));
        vel += (gravity + drift * 2.0f + force) * dt;
        pos += vel * dt;
        p.color.w *= (1.0f - age_ratio);
    }
    else if (p.type == G_TYPE_SPARK) {
        float3 curl = curlNoise(pos * 2.0f, t * 2.0f, cp);
        vel += (make_float3(0.0f, 2.0f, 0.0f) * buoyancy + curl * 10.0f + force) * dt;
        pos += vel * dt;
        p.color = make_float4(1.0f, 0.8f, 0.2f, 1.0f);
        p.color.w *= (1.0f - age_ratio);
    }
    else if (p.type >= 10u) {
        // Material comburente: modelo de combustión de 5 fases (portado de la
        // versión OpenGL del compañero). burnDuration fijo por material — antes
        // el bug de maxLifetime=10000 impedía que ardiera.
        if (p.type == G_TYPE_PAPER) {
            // El papel ondea en el aire antes de quemarse.
            float wave = sinf(p.position.x * 3.0f + t * 2.0f) * cosf(p.position.z * 3.0f + t * 1.5f);
            pos.y += wave * 0.1f * dt;
        }

        float burnStartTime = p.velocity.w;
        if (t > burnStartTime) {
            float burnDuration = 3.0f;                       // papel
            if (p.type == G_TYPE_WOOD)   burnDuration = 15.0f;
            if (p.type == G_TYPE_CEMENT) burnDuration = 50.0f;

            float phase = (t - burnStartTime) / burnDuration;
            float3 rgb = make_float3(p.color.x, p.color.y, p.color.z);

            if (p.type == G_TYPE_PAPER) {
                // ========== FRENTE DE CARBONIZACIÓN DEL PAPEL ==========
                // Borde incandescente que avanza dejando papel NEGRO carbonizado
                // detrás, encogiéndose y curvándose. Es el "cómo se quema un papel".
                float3 dry   = make_float3(0.85f, 0.78f, 0.55f);
                float3 ember = make_float3(1.7f, 0.55f, 0.12f);    // >1 => brilla (bloom)
                float3 charc = make_float3(0.035f, 0.025f, 0.02f); // carbón negro

                if (phase < 0.12f) {
                    // 1. Borde activo: se enciende de seco a naranja incandescente.
                    rgb = mix3(dry, ember, phase / 0.12f);
                } else if (phase < 0.38f) {
                    // 2. El borde colapsa rápido a carbón negro.
                    rgb = mix3(ember, charc, (phase - 0.12f) / 0.26f);
                    p.position.w *= 0.997f;
                } else if (phase < 0.75f) {
                    // 3. Carbón negro con brasa tenue que se apaga; el papel se curva.
                    float tt = (phase - 0.38f) / 0.37f;
                    float pulse = fabsf(sinf(t * 6.0f + pos.x * 25.0f)) * 0.5f + 0.5f;
                    rgb = charc + make_float3(0.45f, 0.10f, 0.0f) * pulse * (1.0f - tt);
                    p.position.w *= 0.996f;
                    pos += curlNoise(pos, t * 0.5f, cp) * 0.004f;
                } else {
                    // 4. Desintegración: POCA llama, mucha ceniza/muerte -> sin confeti.
                    float h = fractf(sinf(pos.x * 12.9898f + pos.z * 78.233f) * 43758.5453f);
                    if (h < 0.15f) {
                        p.type = G_TYPE_FIRE; p.temperature = 0.7f;
                        p.maxLifetime = 0.5f + fractf(h * 20.0f) * 0.5f; p.lifetime = p.maxLifetime;
                        p.velocity.w = 1.0f;
                        vel = vel + make_float3(0.0f, 1.0f + fractf(h * 10.0f) * 0.6f, 0.0f);
                    } else if (h < 0.45f) {
                        p.type = G_TYPE_ASH;
                        p.maxLifetime = 2.0f + fractf(h * 10.0f); p.lifetime = p.maxLifetime;
                        p.color = make_float4(0.04f, 0.04f, 0.04f, 0.75f);
                        vel = vel + make_float3((fractf(h * 15.0f) - 0.5f) * 0.6f,
                                                0.7f + fractf(h * 3.0f) * 0.5f,
                                                (fractf(h * 27.0f) - 0.5f) * 0.6f);
                    } else {
                        p.type = G_TYPE_DEAD;
                    }
                }
                if (phase < 0.75f) { p.color.x = rgb.x; p.color.y = rgb.y; p.color.z = rgb.z; }
            } else {
                // ========== MADERA / CEMENTO: 5 fases + colapso ==========
                // COLAPSO: la madera muy quemada pierde sujeción y cae con gravedad.
                if (p.type == G_TYPE_WOOD && phase > 0.5f) {
                    vel.y -= 7.0f * dt;
                    pos += vel * dt;
                }

                if (phase < 0.1f) {
                    rgb = mix3(rgb, make_float3(0.8f, 0.7f, 0.2f), phase / 0.1f);
                } else if (phase < 0.3f) {
                    rgb = mix3(make_float3(0.8f, 0.7f, 0.2f), make_float3(0.15f, 0.08f, 0.02f), (phase - 0.1f) / 0.2f);
                    p.position.w *= 0.999f;
                } else if (phase < 0.6f) {
                    float pulse = fabsf(sinf(t * 10.0f + pos.x * 20.0f)) * 0.5f + 0.5f;
                    float3 glow = mix3(make_float3(0.8f, 0.2f, 0.0f), make_float3(1.0f, 0.4f, 0.0f), pulse);
                    rgb = mix3(make_float3(0.15f, 0.08f, 0.02f), glow, (phase - 0.3f) / 0.3f);
                    p.position.w *= 0.998f;
                    pos += curlNoise(pos, t * 0.5f, cp) * 0.005f;
                } else if (phase < 1.0f) {
                    rgb = mix3(make_float3(1.0f, 0.4f, 0.0f), make_float3(0.02f, 0.02f, 0.02f), (phase - 0.6f) / 0.4f);
                    p.color.w = mixf(1.0f, 0.7f, (phase - 0.6f) / 0.4f);
                } else {
                    // Desintegración con impulsos REDUCIDOS (antes +2..3 = confeti).
                    float h = fractf(sinf(pos.x * 12.9898f + pos.z * 78.233f) * 43758.5453f);
                    if (h < 0.2f) {
                        p.type = G_TYPE_FIRE; p.temperature = 0.9f;
                        p.maxLifetime = 0.8f + fractf(h * 20.0f); p.lifetime = p.maxLifetime;
                        p.velocity.w = 1.0f;
                        vel = vel + make_float3(0.0f, 1.0f + fractf(h * 10.0f) * 0.5f, 0.0f);
                    } else if (h < 0.45f) {
                        p.type = G_TYPE_ASH;
                        p.maxLifetime = 3.0f + fractf(h * 10.0f); p.lifetime = p.maxLifetime;
                        p.color = make_float4(0.05f, 0.05f, 0.05f, 0.8f);
                        vel = vel + make_float3((fractf(h * 15.0f) - 0.5f) * 0.8f,
                                                1.0f + fractf(h * 3.0f) * 0.5f,
                                                (fractf(h * 27.0f) - 0.5f) * 0.8f);
                    } else {
                        p.type = G_TYPE_DEAD;
                    }
                }
                if (phase < 1.0f) { p.color.x = rgb.x; p.color.y = rgb.y; p.color.z = rgb.z; }
            }
        }
    }

    vel = vel * 0.99f; // fricción

    p.position.x = pos.x; p.position.y = pos.y; p.position.z = pos.z;
    p.velocity.x = vel.x; p.velocity.y = vel.y; p.velocity.z = vel.z;
    P[idx] = p;
}

// --- RNG por hash (igual que particle_emit.comp) ---
__device__ inline unsigned int hashU(unsigned int x) {
    x = (x ^ 61u) ^ (x >> 16u);
    x *= 9u;
    x = x ^ (x >> 4u);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15u);
    return x;
}
__device__ inline float random01(unsigned int& seed) {
    seed = hashU(seed);
    return (float)seed / 4294967295.0f;
}
__device__ inline float3 randomPointInSphere(unsigned int& seed) {
    float theta = random01(seed) * 6.2831853f;
    float z = random01(seed) * 2.0f - 1.0f;
    float r = sqrtf(fmaxf(0.0f, 1.0f - z * z));
    float radius = powf(random01(seed), 1.0f / 3.0f);
    return make_float3(radius * r * cosf(theta), radius * z, radius * r * sinf(theta));
}
__device__ inline float3 randomPointInDisk(unsigned int& seed, float radius) {
    float theta = random01(seed) * 6.2831853f;
    float r = sqrtf(random01(seed)) * radius;
    return make_float3(cosf(theta) * r, 0.0f, sinf(theta) * r);
}
__device__ inline float3 randomPointInRect(unsigned int& seed, float w, float h) {
    return make_float3((random01(seed) - 0.5f) * w, 0.0f, (random01(seed) - 0.5f) * h);
}
__device__ inline float3 randomPointInLine(unsigned int& seed, float w) {
    return make_float3((random01(seed) - 0.5f) * w, 0.0f, 0.0f);
}

__global__ void emitKernel(GpuParticle* P, const GpuEmitter* E, int maxParticles,
                           int numEmitters, float dt, float time) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= maxParticles || numEmitters == 0) return;

    if (P[idx].type != G_TYPE_DEAD) return;

    unsigned int seed = (unsigned int)idx ^ __float_as_uint(time * 1000.0f) ^ 0x9E3779B9u;

    int emitterIndex = idx % numEmitters;
    GpuEmitter em = E[emitterIndex];

    float emitRate = em.directionAndRate.w;
    float pool = (float)maxParticles / (float)(numEmitters > 0 ? numEmitters : 1);
    float probability = clampf((emitRate * dt) / pool, 0.0f, 1.0f);
    if (random01(seed) > probability) return;

    unsigned int shape = (unsigned int)(em.positionAndShape.w + 0.5f);
    float3 spawnPos = make_float3(em.positionAndShape.x, em.positionAndShape.y, em.positionAndShape.z);

    float width = em.dimensions.x, height = em.dimensions.y, radius = em.dimensions.z, particleLife = em.dimensions.w;
    float initialSpeed = em.speedAndTemp.x, speedVariance = em.speedAndTemp.y;
    float temperature = em.speedAndTemp.z, particleSize = em.speedAndTemp.w;

    if (shape == G_SHAPE_DISK)        spawnPos += randomPointInDisk(seed, radius);
    else if (shape == G_SHAPE_RECTANGLE) spawnPos += randomPointInRect(seed, width, height);
    else if (shape == G_SHAPE_LINE)   spawnPos += randomPointInLine(seed, width);
    else if (shape == G_SHAPE_SPHERE) spawnPos += randomPointInSphere(seed) * radius;

    float3 dir = make_float3(em.directionAndRate.x, em.directionAndRate.y, em.directionAndRate.z);
    if (length(dir) < 0.0001f) dir = make_float3(0.0f, 1.0f, 0.0f);
    dir = normalize(dir);

    float3 noiseDir = make_float3(random01(seed) - 0.5f, random01(seed) * 0.5f, random01(seed) - 0.5f);
    dir = normalize(dir + noiseDir * 0.18f); // menos dispersión -> fuego más concentrado

    float speed = initialSpeed + (random01(seed) - 0.5f) * speedVariance;

    GpuParticle p;
    p.position = make_float4(spawnPos.x, spawnPos.y, spawnPos.z, fmaxf(particleSize, 0.03f));
    p.velocity = make_float4(dir.x * speed, dir.y * speed, dir.z * speed, 1.0f);

    float r = random01(seed);
    if (r < 0.80f) {
        p.type = G_TYPE_FIRE;
        p.color = make_float4(1.0f, 0.6f, 0.12f, 1.0f);
        p.temperature = fmaxf(temperature, 0.85f);
        p.maxLifetime = fmaxf(particleLife, 0.8f) * (0.7f + random01(seed) * 0.5f);
    } else {
        // Sin SPARK: los puntos brillantes voladores eran el "confeti". El humo
        // gris oscuro de papel sube y enmarca la llama contra el fondo.
        p.type = G_TYPE_SMOKE;
        p.color = make_float4(0.16f, 0.15f, 0.15f, 0.6f);
        p.temperature = 0.3f;
        p.maxLifetime = fmaxf(particleLife * 1.8f, 2.2f);
        p.position.w *= 2.0f;
    }
    p.lifetime = p.maxLifetime;

    P[idx] = p;
}

// ===========================================================================
// RASTERIZADO POR SPLATTING
// ===========================================================================
__global__ void splatKernel(const GpuParticle* P, int n, Mat4 vp, int W, int H,
                            float proj11, float glow, int showGeometry, int renderSmoke,
                            float4* emissive, float4* smoke, float* smokeW) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    GpuParticle p = P[idx];
    if (p.type == G_TYPE_DEAD) return;
    if (p.type == G_TYPE_SMOKE && !renderSmoke) return; // --no-smoke

    // Clasificar
    bool isEmissive = (p.type == G_TYPE_FIRE || p.type == G_TYPE_EMBER || p.type == G_TYPE_SPARK);
    bool isSmoke    = (p.type == G_TYPE_SMOKE || p.type == G_TYPE_ASH);
    bool isStatic   = (p.type >= 10u); // material (combustible o estructural)
    bool isStructural = (p.type == G_TYPE_CEMENT || p.type == G_TYPE_GROUND);
    if (isStructural && !showGeometry) return; // estructural solo en preview; combustible siempre

    float3 pos = make_float3(p.position.x, p.position.y, p.position.z);
    float4 clip = mulMat4Point(vp, pos);
    if (clip.w <= 1e-4f) return; // detrás de la cámara

    float invw = 1.0f / clip.w;
    float ndcx = clip.x * invw;
    float ndcy = clip.y * invw;
    if (ndcx < -1.3f || ndcx > 1.3f || ndcy < -1.3f || ndcy > 1.3f) return;

    float sx = (ndcx * 0.5f + 0.5f) * (float)W;
    float sy = (1.0f - (ndcy * 0.5f + 0.5f)) * (float)H; // flip Y -> origen arriba-izquierda

    float worldSize = p.position.w;
    float pr = worldSize * proj11 * (0.5f * (float)H) * invw;
    pr = clampf(pr, 0.75f, 180.0f);
    float invr2 = 1.0f / (pr * pr);

    float3 rgb;
    float alpha;
    if (isEmissive) {
        rgb = make_float3(p.color.x, p.color.y, p.color.z) * glow;
        alpha = p.color.w;
    } else { // smoke o material estático
        rgb = make_float3(p.color.x, p.color.y, p.color.z);
        alpha = isStatic ? 1.0f : p.color.w;
    }
    if (alpha <= 0.001f) return;

    int x0 = max(0, (int)floorf(sx - pr));
    int x1 = min(W - 1, (int)ceilf(sx + pr));
    int y0 = max(0, (int)floorf(sy - pr));
    int y1 = min(H - 1, (int)ceilf(sy + pr));

    for (int py = y0; py <= y1; ++py) {
        float dy = (py + 0.5f) - sy;
        for (int px = x0; px <= x1; ++px) {
            float dx = (px + 0.5f) - sx;
            float d2 = (dx * dx + dy * dy) * invr2;
            if (d2 > 1.0f) continue;
            float f = 1.0f - d2;
            f = f * f; // falloff radial suave
            int pix = py * W + px;
            if (isEmissive) {
                atomicAdd(&emissive[pix].x, rgb.x * alpha * f);
                atomicAdd(&emissive[pix].y, rgb.y * alpha * f);
                atomicAdd(&emissive[pix].z, rgb.z * alpha * f);
                atomicAdd(&emissive[pix].w, alpha * f);
            } else {
                float aw = alpha * f;
                atomicAdd(&smoke[pix].x, rgb.x * aw);
                atomicAdd(&smoke[pix].y, rgb.y * aw);
                atomicAdd(&smoke[pix].z, rgb.z * aw);
                atomicAdd(&smoke[pix].w, aw);
                atomicAdd(&smokeW[pix], f);
            }
        }
    }
}

// Resuelve emissive + humo (weighted-average OIT) en un buffer HDR premultiplicado.
__global__ void resolveKernel(const float4* emissive, const float4* smoke, const float* smokeW,
                              float4* hdr, int W, int H, float smokeDensity) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= W * H) return;

    float4 e = emissive[idx];
    float4 sm = smoke[idx];

    float3 smokeColor = make_float3(0.0f, 0.0f, 0.0f);
    if (sm.w > 1e-5f) smokeColor = make_float3(sm.x, sm.y, sm.z) / sm.w;
    float smokeAlpha = clampf(1.0f - expf(-sm.w * smokeDensity), 0.0f, 1.0f);

    float emissiveCoverage = clampf(1.0f - expf(-e.w), 0.0f, 1.0f);

    float3 premult = smokeColor * smokeAlpha + make_float3(e.x, e.y, e.z);
    float outA = clampf(smokeAlpha + emissiveCoverage * (1.0f - smokeAlpha), 0.0f, 1.0f);

    hdr[idx] = make_float4(premult.x, premult.y, premult.z, outA);
}

// ===========================================================================
// RASTERIZADO TILED (estilo 2D Gaussian Splatting, adaptado de rasterizado_cuda)
// Pipeline: preprocess -> scan -> duplicate -> sort -> ranges -> blend tiled.
// Modelo unificado de composición emisión-absorción front-to-back:
//   C += T * alpha_i * color_i ;  T *= (1 - alpha_i)
// Esto cubre fuego (emisivo brillante) y humo (ocluye) con un solo blend, y
// resuelve el orden correctamente (a diferencia del splat aditivo simple).
// ===========================================================================
#define TILED_BATCH 256

// Paso 1: proyecta cada partícula a una gaussiana 2D y cuenta tiles cubiertos.
__global__ void preprocessKernel(const GpuParticle* P, int n, Mat4 vp, int W, int H,
                                 float proj11, float glow, int showGeometry, float smokeDensity,
                                 float sizeScale, int renderSmoke,
                                 int tileSize, int tilesX, int tilesY,
                                 float2* mu, float* sigma, float* depth, float* opacity,
                                 float4* color, int4* tileBox, int* tileCount) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;
    tileCount[idx] = 0;

    GpuParticle p = P[idx];
    if (p.type == G_TYPE_DEAD) return;
    if (p.type == G_TYPE_SMOKE && !renderSmoke) return; // --no-smoke

    bool isEmissive = (p.type == G_TYPE_FIRE || p.type == G_TYPE_EMBER || p.type == G_TYPE_SPARK);
    bool isSmoke    = (p.type == G_TYPE_SMOKE || p.type == G_TYPE_ASH);
    // Material combustible (papel/madera/hoja) ES el sujeto que arde -> se dibuja siempre.
    // Material estructural (cemento/suelo) es contexto para composición -> solo con --show-geometry.
    bool isStructural = (p.type == G_TYPE_CEMENT || p.type == G_TYPE_GROUND);
    if (isStructural && !showGeometry) return;

    float3 pos = make_float3(p.position.x, p.position.y, p.position.z);
    float4 clip = mulMat4Point(vp, pos);
    if (clip.w <= 1e-4f) return;

    float invw = 1.0f / clip.w;
    float ndcx = clip.x * invw, ndcy = clip.y * invw;
    if (ndcx < -1.3f || ndcx > 1.3f || ndcy < -1.3f || ndcy > 1.3f) return;

    float sx = (ndcx * 0.5f + 0.5f) * (float)W;
    float sy = (1.0f - (ndcy * 0.5f + 0.5f)) * (float)H;

    float worldSize = p.position.w;
    float pr = worldSize * proj11 * (0.5f * (float)H) * invw * sizeScale;
    pr = clampf(pr, 1.0f, 180.0f);
    float sig = fmaxf(pr * 0.6f, 0.6f); // radio visual ~ 2*sigma (un poco más suave para fundir)

    float3 rgb;
    float op;
    float emisFlag = 0.0f;
    if (isEmissive) {
        rgb = make_float3(p.color.x, p.color.y, p.color.z) * glow;
        // Opacidad baja: muchas partículas semitransparentes se SUMAN en una llama
        // continua (núcleo brillante, bordes suaves) en vez de puntos duros.
        op = clampf(p.color.w, 0.0f, 1.0f) * 0.35f;
        emisFlag = 1.0f;
    } else if (isSmoke) {
        rgb = make_float3(p.color.x, p.color.y, p.color.z);
        op = clampf(p.color.w * smokeDensity, 0.0f, 0.95f);
    } else { // material (combustible/estructural)
        rgb = make_float3(p.color.x, p.color.y, p.color.z);
        op = 1.0f;
    }
    if (op <= 0.003f) return;

    // Caja de tiles cubierta (extensión ~ pr px alrededor del centro).
    int x0 = max(0, (int)floorf(sx - pr));
    int x1 = min(W - 1, (int)ceilf(sx + pr));
    int y0 = max(0, (int)floorf(sy - pr));
    int y1 = min(H - 1, (int)ceilf(sy + pr));
    if (x1 < x0 || y1 < y0) return;

    int tx0 = x0 / tileSize, tx1 = x1 / tileSize;
    int ty0 = y0 / tileSize, ty1 = y1 / tileSize;
    tx1 = min(tx1, tilesX - 1);
    ty1 = min(ty1, tilesY - 1);

    mu[idx] = make_float2(sx, sy);
    sigma[idx] = sig;
    depth[idx] = clip.w;
    opacity[idx] = op;
    color[idx] = make_float4(rgb.x, rgb.y, rgb.z, emisFlag); // w = flag emisivo
    tileBox[idx] = make_int4(tx0, ty0, tx1, ty1);
    tileCount[idx] = (tx1 - tx0 + 1) * (ty1 - ty0 + 1);
}

// Paso 3: por cada (partícula, tile) escribe clave (tile<<32 | depth) y valor (idx).
__global__ void duplicateKernel(int n, const int* tileOffset, const int* tileCount,
                                const int4* tileBox, const float* depth, int tilesX,
                                unsigned long long* keys, int* vals) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;
    int cnt = tileCount[idx];
    if (cnt == 0) return;

    int base = tileOffset[idx];
    int4 b = tileBox[idx];
    // depth >= 0 (clip.w): el patrón de bits de un float positivo es monótono,
    // así que ordenar por bits ordena front-to-back.
    unsigned int dbits = __float_as_uint(depth[idx]);

    int k = 0;
    for (int ty = b.y; ty <= b.w; ++ty) {
        for (int tx = b.x; tx <= b.z; ++tx) {
            int tileId = ty * tilesX + tx;
            keys[base + k] = ((unsigned long long)tileId << 32) | (unsigned long long)dbits;
            vals[base + k] = idx;
            ++k;
        }
    }
}

// Paso 5: a partir de las claves ordenadas, encuentra el rango [start,end) por tile.
__global__ void rangeKernel(const unsigned long long* keys, int total, int2* ranges) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= total) return;
    int tile = (int)(keys[i] >> 32);
    if (i == 0 || (int)(keys[i - 1] >> 32) != tile) ranges[tile].x = i;
    if (i == total - 1 || (int)(keys[i + 1] >> 32) != tile) ranges[tile].y = i + 1;
}

// Paso 6: blend por tile. 1 bloque = 1 tile, blockDim = tileSize*tileSize = 1 hilo/pixel.
__global__ void __launch_bounds__(256) tiledBlendKernel(
        const float2* mu, const float* sigma, const float* opacity, const float4* color,
        const int* vals, const int2* ranges, float4* hdr, int W, int H, int tileSize, int tilesX) {
    int tileId = blockIdx.x;
    int tid = threadIdx.x;
    int lx = tid % tileSize, ly = tid / tileSize;
    int tx = tileId % tilesX, ty = tileId / tilesX;
    int col = tx * tileSize + lx;
    int row = ty * tileSize + ly;
    bool active = (col < W && row < H);

    int2 rg = ranges[tileId];
    int start = rg.x, end = rg.y;
    if (end <= start) {
        if (active) hdr[row * W + col] = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    float px = (float)col + 0.5f, py = (float)row + 0.5f;
    float3 C = make_float3(0.0f, 0.0f, 0.0f);
    float T = 1.0f;
    bool done = !active;

    __shared__ float s_mux[TILED_BATCH];
    __shared__ float s_muy[TILED_BATCH];
    __shared__ float s_sig[TILED_BATCH];
    __shared__ float s_op[TILED_BATCH];
    __shared__ float s_cr[TILED_BATCH];
    __shared__ float s_cg[TILED_BATCH];
    __shared__ float s_cb[TILED_BATCH];
    __shared__ float s_emis[TILED_BATCH]; // 1 = emisivo (fuego, aditivo) ; 0 = ocluye (humo/madera)

    const float T_MIN = 1e-4f;

    for (int bstart = start; bstart < end; bstart += TILED_BATCH) {
        int bc = end - bstart;
        if (bc > TILED_BATCH) bc = TILED_BATCH;

        for (int l = tid; l < bc; l += blockDim.x) {
            int id = vals[bstart + l];
            s_mux[l] = mu[id].x; s_muy[l] = mu[id].y;
            s_sig[l] = sigma[id]; s_op[l] = opacity[id];
            float4 c = color[id];
            s_cr[l] = c.x; s_cg[l] = c.y; s_cb[l] = c.z; s_emis[l] = c.w;
        }
        __syncthreads();

        if (!done) {
            for (int k = 0; k < bc; ++k) {
                float dx = px - s_mux[k], dy = py - s_muy[k];
                float sig = s_sig[k];
                float e = (dx * dx + dy * dy) / (2.0f * sig * sig);
                if (e > 9.0f) continue;               // fuera de ~3 sigma
                float alpha = s_op[k] * expf(-e);
                if (alpha < 1e-4f) continue;
                alpha = fminf(alpha, 0.999f);
                // El fuego SUMA luz (aditivo) y no ocluye -> se funde en llama con
                // núcleo brillante y bordes suaves. El humo/madera sí ocluyen.
                float w = alpha * T;
                C.x += w * s_cr[k]; C.y += w * s_cg[k]; C.z += w * s_cb[k];
                if (s_emis[k] < 0.5f) {
                    T *= (1.0f - alpha);
                    if (T < T_MIN) { done = true; break; }
                }
            }
        }
        if (__syncthreads_count(done) == blockDim.x) break;
    }

    if (active) hdr[row * W + col] = make_float4(C.x, C.y, C.z, 1.0f - T);
}

// Bright pass para el bloom (a partir del buffer emisivo).
__global__ void brightKernel(const float4* emissive, float4* bloom, int W, int H, float threshold) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= W * H) return;
    float4 e = emissive[idx];
    float lum = 0.2126f * e.x + 0.7152f * e.y + 0.0722f * e.z;
    float factor = lum > threshold ? (lum - threshold) / fmaxf(lum, 1e-4f) : 0.0f;
    bloom[idx] = make_float4(e.x * factor, e.y * factor, e.z * factor, 0.0f);
}

// Blur gaussiano separable (9 taps).
__global__ void blurKernel(const float4* src, float4* dst, int W, int H, int horizontal) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= W || y >= H) return;

    const float wgt[5] = {0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f};
    float3 acc = make_float3(src[y * W + x].x, src[y * W + x].y, src[y * W + x].z) * wgt[0];
    for (int i = 1; i < 5; ++i) {
        int dx = horizontal ? i : 0;
        int dy = horizontal ? 0 : i;
        int xp = min(W - 1, x + dx), yp = min(H - 1, y + dy);
        int xm = max(0, x - dx), ym = max(0, y - dy);
        float4 a = src[yp * W + xp];
        float4 b = src[ym * W + xm];
        acc += (make_float3(a.x, a.y, a.z) + make_float3(b.x, b.y, b.z)) * wgt[i];
    }
    dst[y * W + x] = make_float4(acc.x, acc.y, acc.z, 0.0f);
}

__device__ inline float3 acesTonemap(float3 x) {
    const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    float3 num = x * (a * x + make_float3(b, b, b));
    float3 den = x * (c * x + make_float3(d, d, d)) + make_float3(e, e, e);
    return make_float3(clampf(num.x / den.x, 0.0f, 1.0f),
                       clampf(num.y / den.y, 0.0f, 1.0f),
                       clampf(num.z / den.z, 0.0f, 1.0f));
}
__device__ inline float srgb(float c) {
    c = clampf(c, 0.0f, 1.0f);
    return c <= 0.0031308f ? 12.92f * c : 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
}

__global__ void compositeKernel(const float4* hdr, const float4* bloom, uchar4* out,
                                int W, int H, float exposure, float bloomIntensity,
                                int solidBg, float3 bgColor) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= W * H) return;

    float4 h = hdr[idx];
    float4 bl = bloom[idx];
    // color premultiplicado (fuego + glow)
    float3 color = make_float3(h.x, h.y, h.z) + make_float3(bl.x, bl.y, bl.z) * bloomIntensity;
    float alpha = clampf(h.w, 0.0f, 1.0f);

    if (solidBg) {
        // Componer sobre fondo sólido: out = fuego_premult + bg*(1-alpha)
        color = color + bgColor * (1.0f - alpha);
    }

    color = color * exposure;
    color = acesTonemap(color);

    unsigned char r = (unsigned char)(srgb(color.x) * 255.0f + 0.5f);
    unsigned char g = (unsigned char)(srgb(color.y) * 255.0f + 0.5f);
    unsigned char b = (unsigned char)(srgb(color.z) * 255.0f + 0.5f);
    unsigned char a = solidBg ? 255 : (unsigned char)(alpha * 255.0f + 0.5f);
    out[idx] = make_uchar4(r, g, b, a);
}

// ===========================================================================
// API HOST
// ===========================================================================
FireEngine::FireEngine() {}
FireEngine::~FireEngine() { shutdown(); }

void FireEngine::init(int width, int height,
                      const std::vector<Particle>& initialParticles,
                      const std::vector<EmitterConfig>& emitters,
                      const CurlNoiseParams& curl,
                      const glm::vec3& windDir, float windStrength) {
    width_ = width;
    height_ = height;
    maxParticles_ = (int)initialParticles.size();
    numEmitters_ = (int)emitters.size();
    windDir_ = windStrength > 0.0f ? glm::normalize(windDir) : glm::vec3(0.0f);
    windStrength_ = windStrength;

    curl_[0] = curl.frequency; curl_[1] = curl.amplitude; curl_[2] = (float)curl.octaves;
    curl_[3] = curl.lacunarity; curl_[4] = curl.persistence; curl_[5] = curl.timeScale; curl_[6] = curl.epsilon;

    static_assert(sizeof(Particle) == 64, "Particle (host) debe medir 64 bytes para coincidir con GpuParticle");

    // Partículas: copia binaria directa (layout idéntico).
    size_t pBytes = (size_t)maxParticles_ * sizeof(GpuParticle);
    CUDA_CHECK(cudaMalloc(&d_particles_, pBytes));
    if (maxParticles_ > 0)
        CUDA_CHECK(cudaMemcpy(d_particles_, initialParticles.data(), pBytes, cudaMemcpyHostToDevice));

    // Emisores: empaquetar como GpuEmitter.
    if (numEmitters_ > 0) {
        std::vector<GpuEmitter> gpuEms(numEmitters_);
        for (int i = 0; i < numEmitters_; ++i) {
            const EmitterConfig& em = emitters[i];
            gpuEms[i].positionAndShape = make_float4(em.position.x, em.position.y, em.position.z, (float)em.shape);
            gpuEms[i].directionAndRate = make_float4(em.direction.x, em.direction.y, em.direction.z, em.emitRate);
            gpuEms[i].dimensions       = make_float4(em.width, em.height, em.radius, em.particleLife);
            gpuEms[i].speedAndTemp     = make_float4(em.initialSpeed, em.speedVariance, em.temperature, em.particleSize);
        }
        size_t eBytes = (size_t)numEmitters_ * sizeof(GpuEmitter);
        CUDA_CHECK(cudaMalloc(&d_emitters_, eBytes));
        CUDA_CHECK(cudaMemcpy(d_emitters_, gpuEms.data(), eBytes, cudaMemcpyHostToDevice));
    }

    // Buffers de render.
    size_t px = (size_t)width_ * height_;
    CUDA_CHECK(cudaMalloc(&d_emissive_, px * sizeof(float4)));
    CUDA_CHECK(cudaMalloc(&d_smoke_,    px * sizeof(float4)));
    CUDA_CHECK(cudaMalloc(&d_smokeW_,   px * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_hdr_,      px * sizeof(float4)));
    CUDA_CHECK(cudaMalloc(&d_bloomA_,   px * sizeof(float4)));
    CUDA_CHECK(cudaMalloc(&d_bloomB_,   px * sizeof(float4)));
    CUDA_CHECK(cudaMalloc(&d_out_,      px * sizeof(uchar4)));
    CUDA_CHECK(cudaHostAlloc(&h_pinned_, px * 4, cudaHostAllocDefault));

    // Buffers del rasterizador tiled.
    tilesX_ = (width_  + kTileSize - 1) / kTileSize;
    tilesY_ = (height_ + kTileSize - 1) / kTileSize;
    numTiles_ = tilesX_ * tilesY_;
    if (maxParticles_ > 0) {
        int n = maxParticles_;
        CUDA_CHECK(cudaMalloc(&d_visMu_,      n * sizeof(float2)));
        CUDA_CHECK(cudaMalloc(&d_visSigma_,   n * sizeof(float)));
        CUDA_CHECK(cudaMalloc(&d_visDepth_,   n * sizeof(float)));
        CUDA_CHECK(cudaMalloc(&d_visOpacity_, n * sizeof(float)));
        CUDA_CHECK(cudaMalloc(&d_visColor_,   n * sizeof(float4)));
        CUDA_CHECK(cudaMalloc(&d_tileBox_,    n * sizeof(int4)));
        CUDA_CHECK(cudaMalloc(&d_tileCount_,  n * sizeof(int)));
        CUDA_CHECK(cudaMalloc(&d_tileOffset_, n * sizeof(int)));
    }
    CUDA_CHECK(cudaMalloc(&d_ranges_, (size_t)numTiles_ * sizeof(int2)));
    // d_keys_/d_vals_ se asignan de forma perezosa según el nº de pares por frame.

    std::printf("[FireEngine] init %dx%d, %d particulas, %d emisores | tiles %dx%d (%d)\n",
                width_, height_, maxParticles_, numEmitters_, tilesX_, tilesY_, numTiles_);
}

void FireEngine::update(float dt, float simTime) {
    if (maxParticles_ == 0) return;
    GpuCurlParams cp;
    cp.frequency = curl_[0]; cp.amplitude = curl_[1]; cp.octaves = (int)curl_[2];
    cp.lacunarity = curl_[3]; cp.persistence = curl_[4]; cp.timeScale = curl_[5]; cp.epsilon = curl_[6];

    float3 g = make_float3(gravity_.x, gravity_.y, gravity_.z);
    float3 wd = make_float3(windDir_.x, windDir_.y, windDir_.z);

    int grid = (maxParticles_ + BLOCK - 1) / BLOCK;
    updateKernel<<<grid, BLOCK>>>((GpuParticle*)d_particles_, maxParticles_, dt, simTime,
                                  cp, g, buoyancy_, wd, windStrength_);
    if (numEmitters_ > 0) {
        emitKernel<<<grid, BLOCK>>>((GpuParticle*)d_particles_, (GpuEmitter*)d_emitters_,
                                    maxParticles_, numEmitters_, dt, simTime);
    }
    CUDA_CHECK(cudaGetLastError());
    frame_++;
}

void FireEngine::render(const glm::mat4& viewProj, float proj11,
                        const RenderParams& rp, std::vector<uint8_t>& outRGBA) {
    size_t px = (size_t)width_ * height_;
    int gridPix = ((int)px + BLOCK - 1) / BLOCK;
    int gridP = (maxParticles_ + BLOCK - 1) / BLOCK;

    Mat4 vp;
    const float* src = &viewProj[0][0]; // glm column-major
    for (int i = 0; i < 16; ++i) vp.m[i] = src[i];

    if (rp.tiled && maxParticles_ > 0) {
        // ---------- Rasterizador TILED (2DGS) ----------
        // 1. Preprocess: gaussiana 2D + nº de tiles por partícula.
        preprocessKernel<<<gridP, BLOCK>>>(
            (GpuParticle*)d_particles_, maxParticles_, vp, width_, height_,
            proj11, rp.particleGlow, rp.showGeometry ? 1 : 0, rp.smokeDensity,
            rp.sizeScale, rp.renderSmoke ? 1 : 0,
            kTileSize, tilesX_, tilesY_,
            (float2*)d_visMu_, (float*)d_visSigma_, (float*)d_visDepth_, (float*)d_visOpacity_,
            (float4*)d_visColor_, (int4*)d_tileBox_, (int*)d_tileCount_);

        // 2. Prefix-sum de counts -> offsets, y total de pares.
        thrust::device_ptr<int> tc((int*)d_tileCount_);
        thrust::device_ptr<int> to((int*)d_tileOffset_);
        thrust::exclusive_scan(tc, tc + maxParticles_, to);

        int lastOff = 0, lastCnt = 0;
        CUDA_CHECK(cudaMemcpy(&lastOff, (int*)d_tileOffset_ + (maxParticles_ - 1), sizeof(int), cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(&lastCnt, (int*)d_tileCount_  + (maxParticles_ - 1), sizeof(int), cudaMemcpyDeviceToHost));
        long long total = (long long)lastOff + lastCnt;

        CUDA_CHECK(cudaMemset(d_ranges_, 0, (size_t)numTiles_ * sizeof(int2)));

        if (total > 0) {
            // Asegurar capacidad de keys/vals.
            if ((size_t)total > pairCapacity_) {
                if (d_keys_) cudaFree(d_keys_);
                if (d_vals_) cudaFree(d_vals_);
                pairCapacity_ = (size_t)(total * 1.3) + 1024;
                CUDA_CHECK(cudaMalloc(&d_keys_, pairCapacity_ * sizeof(unsigned long long)));
                CUDA_CHECK(cudaMalloc(&d_vals_, pairCapacity_ * sizeof(int)));
            }
            // 3. Duplicar pares (partícula, tile).
            duplicateKernel<<<gridP, BLOCK>>>(
                maxParticles_, (int*)d_tileOffset_, (int*)d_tileCount_,
                (int4*)d_tileBox_, (float*)d_visDepth_, tilesX_,
                (unsigned long long*)d_keys_, (int*)d_vals_);

            // 4. Ordenar por (tile, depth).
            thrust::device_ptr<unsigned long long> kp((unsigned long long*)d_keys_);
            thrust::device_ptr<int> vv((int*)d_vals_);
            thrust::sort_by_key(kp, kp + total, vv);

            // 5. Rangos por tile.
            int gridT = ((int)total + BLOCK - 1) / BLOCK;
            rangeKernel<<<gridT, BLOCK>>>((unsigned long long*)d_keys_, (int)total, (int2*)d_ranges_);

            // 6. Blend tiled -> HDR premultiplicado.
            tiledBlendKernel<<<numTiles_, kTileSize * kTileSize>>>(
                (float2*)d_visMu_, (float*)d_visSigma_, (float*)d_visOpacity_, (float4*)d_visColor_,
                (int*)d_vals_, (int2*)d_ranges_, (float4*)d_hdr_, width_, height_, kTileSize, tilesX_);
        } else {
            CUDA_CHECK(cudaMemset(d_hdr_, 0, px * sizeof(float4)));
        }

        brightKernel<<<gridPix, BLOCK>>>((float4*)d_hdr_, (float4*)d_bloomA_, width_, height_, rp.bloomThreshold);
    } else {
        // ---------- Splat simple (fallback) ----------
        CUDA_CHECK(cudaMemset(d_emissive_, 0, px * sizeof(float4)));
        CUDA_CHECK(cudaMemset(d_smoke_,    0, px * sizeof(float4)));
        CUDA_CHECK(cudaMemset(d_smokeW_,   0, px * sizeof(float)));

        splatKernel<<<gridP, BLOCK>>>((GpuParticle*)d_particles_, maxParticles_, vp, width_, height_,
                                      proj11, rp.particleGlow, rp.showGeometry ? 1 : 0, rp.renderSmoke ? 1 : 0,
                                      (float4*)d_emissive_, (float4*)d_smoke_, (float*)d_smokeW_);

        resolveKernel<<<gridPix, BLOCK>>>((float4*)d_emissive_, (float4*)d_smoke_, (float*)d_smokeW_,
                                          (float4*)d_hdr_, width_, height_, rp.smokeDensity);

        brightKernel<<<gridPix, BLOCK>>>((float4*)d_emissive_, (float4*)d_bloomA_, width_, height_, rp.bloomThreshold);
    }

    // Bloom (blur separable ping-pong) + composición (compartido).
    dim3 b2(16, 16);
    dim3 g2((width_ + b2.x - 1) / b2.x, (height_ + b2.y - 1) / b2.y);
    for (int i = 0; i < rp.bloomIterations; ++i) {
        blurKernel<<<g2, b2>>>((float4*)d_bloomA_, (float4*)d_bloomB_, width_, height_, 1);
        blurKernel<<<g2, b2>>>((float4*)d_bloomB_, (float4*)d_bloomA_, width_, height_, 0);
    }

    compositeKernel<<<gridPix, BLOCK>>>((float4*)d_hdr_, (float4*)d_bloomA_, (uchar4*)d_out_,
                                        width_, height_, rp.exposure, rp.bloomIntensity,
                                        rp.solidBackground ? 1 : 0, make_float3(rp.bgR, rp.bgG, rp.bgB));
    CUDA_CHECK(cudaGetLastError());

    CUDA_CHECK(cudaMemcpy(h_pinned_, d_out_, px * 4, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaDeviceSynchronize());

    outRGBA.resize(px * 4);
    std::memcpy(outRGBA.data(), h_pinned_, px * 4);
}

void FireEngine::shutdown() {
    if (d_particles_) cudaFree(d_particles_);
    if (d_emitters_)  cudaFree(d_emitters_);
    if (d_emissive_)  cudaFree(d_emissive_);
    if (d_smoke_)     cudaFree(d_smoke_);
    if (d_smokeW_)    cudaFree(d_smokeW_);
    if (d_hdr_)       cudaFree(d_hdr_);
    if (d_bloomA_)    cudaFree(d_bloomA_);
    if (d_bloomB_)    cudaFree(d_bloomB_);
    if (d_out_)       cudaFree(d_out_);
    if (h_pinned_)    cudaFreeHost(h_pinned_);
    // Buffers del tiled.
    if (d_visMu_)      cudaFree(d_visMu_);
    if (d_visSigma_)   cudaFree(d_visSigma_);
    if (d_visDepth_)   cudaFree(d_visDepth_);
    if (d_visOpacity_) cudaFree(d_visOpacity_);
    if (d_visColor_)   cudaFree(d_visColor_);
    if (d_tileBox_)    cudaFree(d_tileBox_);
    if (d_tileCount_)  cudaFree(d_tileCount_);
    if (d_tileOffset_) cudaFree(d_tileOffset_);
    if (d_keys_)       cudaFree(d_keys_);
    if (d_vals_)       cudaFree(d_vals_);
    if (d_ranges_)     cudaFree(d_ranges_);
    d_particles_ = d_emitters_ = d_emissive_ = d_smoke_ = d_smokeW_ = nullptr;
    d_hdr_ = d_bloomA_ = d_bloomB_ = d_out_ = nullptr;
    h_pinned_ = nullptr;
    d_visMu_ = d_visSigma_ = d_visDepth_ = d_visOpacity_ = d_visColor_ = nullptr;
    d_tileBox_ = d_tileCount_ = d_tileOffset_ = d_keys_ = d_vals_ = d_ranges_ = nullptr;
    pairCapacity_ = 0;
}
