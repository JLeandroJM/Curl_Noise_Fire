#include <metal_stdlib>
using namespace metal;

// ---------------------------------------------------------------------------
//                              SHARED LAYOUT
// (mirrors src/Particle.hpp; sizes are statically asserted on the host side)
// ---------------------------------------------------------------------------

struct Particle {
    packed_float3 position;
    float age;
    packed_float3 velocity;
    float lifetime;
    float size;
    float seed;
    float _pad0;
    float _pad1;
};

struct SimUniforms {
    float dt;
    float time;
    uint  frameIndex;
    uint  particleCount;

    packed_float3 emitterPos;
    float _pad0;

    float emitVelMin;
    float emitVelMax;
    float emitRadius;
    float buoyancy;

    float curlScale;
    float curlStrength;
    float lifetimeMin;
    float lifetimeMax;

    float sizeMin;
    float sizeMax;
    float _pad1;
    float _pad2;
};

struct CameraUniforms {
    float4x4 viewProj;
    packed_float3 cameraRight;
    float _pad0;
    packed_float3 cameraUp;
    float _pad1;
};

// ---------------------------------------------------------------------------
//                       SIMPLEX 3D (Ashima / Gustavson)
// Direct MSL port of webgl-noise/src/noise3D.glsl, which is the same code as
// noise3D.glsl in the repo root. Kept self-contained so the file is hermetic.
// ---------------------------------------------------------------------------

static inline float3 mod289_3(float3 x) { return x - floor(x * (1.0f/289.0f)) * 289.0f; }
static inline float4 mod289_4(float4 x) { return x - floor(x * (1.0f/289.0f)) * 289.0f; }
static inline float4 permute(float4 x)  { return mod289_4(((x*34.0f)+10.0f)*x); }
static inline float4 taylorInvSqrt(float4 r) { return 1.79284291400159f - 0.85373472095314f * r; }

static float snoise(float3 v) {
    const float2 C = float2(1.0f/6.0f, 1.0f/3.0f);
    const float4 D = float4(0.0f, 0.5f, 1.0f, 2.0f);

    // First corner
    float3 i  = floor(v + dot(v, C.yyy));
    float3 x0 = v   - i + dot(i, C.xxx);

    // Other corners
    float3 g  = step(x0.yzx, x0.xyz);
    float3 l  = 1.0f - g;
    float3 i1 = min(g.xyz, l.zxy);
    float3 i2 = max(g.xyz, l.zxy);

    float3 x1 = x0 - i1 + C.xxx;
    float3 x2 = x0 - i2 + C.yyy;
    float3 x3 = x0 - D.yyy;

    // Permutations
    i = mod289_3(i);
    float4 p = permute(permute(permute(
                 i.z + float4(0.0f, i1.z, i2.z, 1.0f))
               + i.y + float4(0.0f, i1.y, i2.y, 1.0f))
               + i.x + float4(0.0f, i1.x, i2.x, 1.0f));

    // Gradients: 7x7 points over a square, mapped onto an octahedron.
    float n_  = 1.0f / 7.0f;
    float3 ns = n_ * D.wyz - D.xzx;

    float4 j = p - 49.0f * floor(p * ns.z * ns.z);

    float4 x_ = floor(j * ns.z);
    float4 y_ = floor(j - 7.0f * x_);

    float4 x = x_ * ns.x + ns.yyyy;
    float4 y = y_ * ns.x + ns.yyyy;
    float4 h = 1.0f - abs(x) - abs(y);

    float4 b0 = float4(x.xy, y.xy);
    float4 b1 = float4(x.zw, y.zw);

    float4 s0 = floor(b0) * 2.0f + 1.0f;
    float4 s1 = floor(b1) * 2.0f + 1.0f;
    float4 sh = -step(h, float4(0.0f));

    float4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    float4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    float3 p0 = float3(a0.xy, h.x);
    float3 p1 = float3(a0.zw, h.y);
    float3 p2 = float3(a1.xy, h.z);
    float3 p3 = float3(a1.zw, h.w);

    // Normalise gradients
    float4 norm = taylorInvSqrt(float4(dot(p0,p0), dot(p1,p1), dot(p2,p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Mix final noise value
    float4 m = max(0.6f - float4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0f);
    m = m * m;
    return 42.0f * dot(m * m, float4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

// ---------------------------------------------------------------------------
//   Curl of a 3D vector potential whose components are decorrelated simplex
//   noises (Bridson 2007). Offsets chosen large enough that the three noise
//   fields are effectively independent.
// ---------------------------------------------------------------------------

static float3 snoiseVec3(float3 x) {
    float s0 = snoise(x);
    float s1 = snoise(float3(x.y - 19.1f, x.z + 33.4f, x.x + 47.2f));
    float s2 = snoise(float3(x.z + 74.2f, x.x - 124.5f, x.y + 99.4f));
    return float3(s0, s1, s2);
}

static float3 curlNoise(float3 p) {
    const float e = 0.1f;
    float3 dx = float3(e, 0.0f, 0.0f);
    float3 dy = float3(0.0f, e, 0.0f);
    float3 dz = float3(0.0f, 0.0f, e);

    float3 p_x0 = snoiseVec3(p - dx);
    float3 p_x1 = snoiseVec3(p + dx);
    float3 p_y0 = snoiseVec3(p - dy);
    float3 p_y1 = snoiseVec3(p + dy);
    float3 p_z0 = snoiseVec3(p - dz);
    float3 p_z1 = snoiseVec3(p + dz);

    float x = (p_y1.z - p_y0.z) - (p_z1.y - p_z0.y);
    float y = (p_z1.x - p_z0.x) - (p_x1.z - p_x0.z);
    float z = (p_x1.y - p_x0.y) - (p_y1.x - p_y0.x);

    const float divisor = 1.0f / (2.0f * e);
    return float3(x, y, z) * divisor;
}

// ---------------------------------------------------------------------------
//                              FAST PRNG
// Hash-based, fully deterministic from (seed, frameIndex). Cheap and good
// enough for jittering respawn positions and velocities.
// ---------------------------------------------------------------------------

static inline uint hash_uint(uint x) {
    x = (x ^ 61u) ^ (x >> 16u);
    x *= 9u;
    x = x ^ (x >> 4u);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15u);
    return x;
}

static inline float hash01(uint x) {
    return float(hash_uint(x)) * (1.0f / 4294967296.0f);
}

// ---------------------------------------------------------------------------
//                          COMPUTE: UPDATE / RECYCLE
// ---------------------------------------------------------------------------

kernel void update_particles(device   Particle*    particles [[buffer(0)]],
                             constant SimUniforms& u         [[buffer(1)]],
                             uint                  gid       [[thread_position_in_grid]])
{
    if (gid >= u.particleCount) return;

    Particle p = particles[gid];

    // Curl noise + buoyancy (Reeves 1983 buoyancy = constant upward accel).
    float3 pos      = float3(p.position);
    float3 vel      = float3(p.velocity);

    float3 curl     = curlNoise(pos * u.curlScale + float3(0.0f, u.time * 0.3f, 0.0f));
    float3 acc      = curl * u.curlStrength;
    acc.y          += u.buoyancy;

    vel += acc * u.dt;

    // Mild damping so the particles don't accumulate velocity forever.
    vel *= (1.0f - 0.6f * u.dt);

    pos += vel * u.dt;

    p.position = pos;
    p.velocity = vel;
    p.age     += u.dt;

    // Respawn dead particles. Each respawn pulls fresh randomness from a
    // hash seeded by (particle seed, frame index) so successive lives of
    // the same slot don't repeat.
    if (p.age >= p.lifetime) {
        uint baseSeed = uint(p.seed) ^ (u.frameIndex * 2654435761u) ^ (gid * 0x9E3779B9u);

        float r0 = hash01(baseSeed +  1u);
        float r1 = hash01(baseSeed +  2u);
        float r2 = hash01(baseSeed +  3u);
        float r3 = hash01(baseSeed +  4u);
        float r4 = hash01(baseSeed +  5u);
        float r5 = hash01(baseSeed +  6u);
        float r6 = hash01(baseSeed +  7u);

        // Jitter inside a small disk around the emitter so the base of the
        // fire isn't a single point.
        float ang = r0 * 6.2831853f;
        float rad = sqrt(r1) * u.emitRadius;
        float3 jitter = float3(cos(ang) * rad, 0.0f, sin(ang) * rad);

        p.position = float3(u.emitterPos) + jitter;

        // Mostly-vertical launch with a small horizontal component.
        float speed = mix(u.emitVelMin, u.emitVelMax, r2);
        float a2    = r3 * 6.2831853f;
        float rH    = r4 * 0.25f;     // small horizontal kick
        p.velocity  = float3(cos(a2) * rH, speed, sin(a2) * rH);

        p.age      = 0.0f;
        p.lifetime = mix(u.lifetimeMin, u.lifetimeMax, r5);
        p.size     = mix(u.sizeMin,     u.sizeMax,     r6);
        // p.seed kept stable across lives, used as identity for hashing.
    }

    particles[gid] = p;
}

// ---------------------------------------------------------------------------
//                       BILLBOARD VERTEX + FRAGMENT
// ---------------------------------------------------------------------------

struct VSOut {
    float4 position [[position]];
    float2 uv;
    float  ageFrac;
};

// Quad corners generated from vertex_id (two triangles, CCW).
constant float2 kCorners[6] = {
    float2(-1.0f, -1.0f),
    float2( 1.0f, -1.0f),
    float2( 1.0f,  1.0f),
    float2(-1.0f, -1.0f),
    float2( 1.0f,  1.0f),
    float2(-1.0f,  1.0f),
};

vertex VSOut billboard_vertex(uint                vid  [[vertex_id]],
                              uint                iid  [[instance_id]],
                              device const Particle* particles [[buffer(0)]],
                              constant CameraUniforms& cam      [[buffer(1)]])
{
    Particle p = particles[iid];

    float2 corner = kCorners[vid];

    float3 right = float3(cam.cameraRight);
    float3 up    = float3(cam.cameraUp);

    // Fade in fast, stay big, fade out toward end of life: size envelope.
    float t = clamp(p.age / max(p.lifetime, 1e-4f), 0.0f, 1.0f);
    float sizeEnv = smoothstep(0.0f, 0.15f, t) * (1.0f - smoothstep(0.8f, 1.0f, t) * 0.6f);
    float worldSize = p.size * (0.6f + sizeEnv * 0.6f);

    float3 worldPos = float3(p.position)
                   + (right * corner.x + up * corner.y) * worldSize;

    VSOut o;
    o.position = cam.viewProj * float4(worldPos, 1.0f);
    o.uv       = corner * 0.5f + 0.5f;
    o.ageFrac  = t;
    return o;
}

// Color gradient by age, intended for additive blending.
fragment float4 billboard_fragment(VSOut in [[stage_in]])
{
    // Soft radial falloff (gaussian-ish).
    float2 d = in.uv - 0.5f;
    float  r = length(d) * 2.0f;       // 0 center -> ~1.41 corner
    float  alpha = exp(-r * r * 3.5f);
    alpha = max(alpha - 0.02f, 0.0f);  // crisp the edges

    float t = in.ageFrac;

    // Hand-tuned palette:
    //   0.00 white-yellow core
    //   0.20 yellow-orange
    //   0.55 deep red
    //   0.85 dim red embers / smoke (close to black -> contributes little
    //        in additive blending, simulating smoke darkening)
    float3 c0 = float3(1.30f, 1.20f, 0.80f);   // hot white-yellow
    float3 c1 = float3(1.20f, 0.65f, 0.15f);   // orange
    float3 c2 = float3(0.85f, 0.18f, 0.05f);   // deep red
    float3 c3 = float3(0.12f, 0.10f, 0.10f);   // smoke

    float3 rgb;
    if (t < 0.2f) {
        rgb = mix(c0, c1, t / 0.2f);
    } else if (t < 0.55f) {
        rgb = mix(c1, c2, (t - 0.2f) / 0.35f);
    } else {
        rgb = mix(c2, c3, smoothstep(0.55f, 1.0f, t));
    }

    // Overall intensity envelope: strongest in mid-life, fades at end.
    float intensity = (1.0f - smoothstep(0.75f, 1.0f, t)) * (0.4f + 0.6f * smoothstep(0.0f, 0.2f, t));

    return float4(rgb * alpha * intensity, alpha * intensity);
}
