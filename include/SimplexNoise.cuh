// =============================================================================
// SimplexNoise.cuh - Port a CUDA del simplex noise 3D/4D (Ashima / Gustavson, MIT)
// Traducción fiel de shaders/simplex_noise.glsl a funciones __device__.
// =============================================================================
#pragma once

#include "vec_math.cuh"

// ---------------------------------------------------------------------------
// mod289 / permute / taylorInvSqrt
// ---------------------------------------------------------------------------
__device__ inline float  mod289f(float x) { return x - floorf(x * (1.0f / 289.0f)) * 289.0f; }
__device__ inline float4 mod289_4(float4 x) {
    return make_float4(mod289f(x.x), mod289f(x.y), mod289f(x.z), mod289f(x.w));
}
__device__ inline float3 mod289_3(float3 x) {
    return make_float3(mod289f(x.x), mod289f(x.y), mod289f(x.z));
}
__device__ inline float4 permute4(float4 x) {
    // mod289(((x*34)+1)*x)
    float4 t = make_float4(x.x * 34.0f + 1.0f, x.y * 34.0f + 1.0f, x.z * 34.0f + 1.0f, x.w * 34.0f + 1.0f);
    return mod289_4(t * x);
}
__device__ inline float permute1(float x) {
    return mod289f((x * 34.0f + 1.0f) * x);
}
__device__ inline float4 taylorInvSqrt4(float4 r) {
    return make_float4(1.79284291400159f - 0.85373472095314f * r.x,
                       1.79284291400159f - 0.85373472095314f * r.y,
                       1.79284291400159f - 0.85373472095314f * r.z,
                       1.79284291400159f - 0.85373472095314f * r.w);
}
__device__ inline float taylorInvSqrt1(float r) {
    return 1.79284291400159f - 0.85373472095314f * r;
}

// ---------------------------------------------------------------------------
// Simplex Noise 3D
// ---------------------------------------------------------------------------
__device__ inline float snoise3(float3 v) {
    const float Cx = 1.0f / 6.0f;
    const float Cy = 1.0f / 3.0f;

    // i = floor(v + dot(v, C.yyy))
    float s = (v.x + v.y + v.z) * Cy;
    float3 i = make_float3(floorf(v.x + s), floorf(v.y + s), floorf(v.z + s));
    // x0 = v - i + dot(i, C.xxx)
    float si = (i.x + i.y + i.z) * Cx;
    float3 x0 = make_float3(v.x - i.x + si, v.y - i.y + si, v.z - i.z + si);

    // g = step(x0.yzx, x0.xyz)
    float3 g = make_float3(x0.x >= x0.y ? 1.0f : 0.0f,
                           x0.y >= x0.z ? 1.0f : 0.0f,
                           x0.z >= x0.x ? 1.0f : 0.0f);
    float3 l = make_float3(1.0f - g.x, 1.0f - g.y, 1.0f - g.z);
    // i1 = min(g.xyz, l.zxy); i2 = max(g.xyz, l.zxy)
    float3 i1 = make_float3(fminf(g.x, l.z), fminf(g.y, l.x), fminf(g.z, l.y));
    float3 i2 = make_float3(fmaxf(g.x, l.z), fmaxf(g.y, l.x), fmaxf(g.z, l.y));

    float3 x1 = make_float3(x0.x - i1.x + Cx, x0.y - i1.y + Cx, x0.z - i1.z + Cx);
    float3 x2 = make_float3(x0.x - i2.x + Cy, x0.y - i2.y + Cy, x0.z - i2.z + Cy);
    float3 x3 = make_float3(x0.x - 0.5f, x0.y - 0.5f, x0.z - 0.5f);

    i = mod289_3(i);
    float4 p = permute4(permute4(permute4(
        make_float4(i.z + 0.0f, i.z + i1.z, i.z + i2.z, i.z + 1.0f))
        + make_float4(i.y + 0.0f, i.y + i1.y, i.y + i2.y, i.y + 1.0f))
        + make_float4(i.x + 0.0f, i.x + i1.x, i.x + i2.x, i.x + 1.0f));

    float n_ = 0.142857142857f; // 1/7
    // ns = n_ * D.wyz - D.xzx ; D.wyz=(2,0.5,1), D.xzx=(0,1,0)
    float3 ns = make_float3(n_ * 2.0f - 0.0f, n_ * 0.5f - 1.0f, n_ * 1.0f - 0.0f);

    // j = p - 49 * floor(p * ns.z * ns.z)
    float nz2 = ns.z * ns.z;
    float4 j = make_float4(p.x - 49.0f * floorf(p.x * nz2),
                           p.y - 49.0f * floorf(p.y * nz2),
                           p.z - 49.0f * floorf(p.z * nz2),
                           p.w - 49.0f * floorf(p.w * nz2));

    float4 x_ = make_float4(floorf(j.x * ns.z), floorf(j.y * ns.z), floorf(j.z * ns.z), floorf(j.w * ns.z));
    float4 y_ = make_float4(floorf(j.x - 7.0f * x_.x), floorf(j.y - 7.0f * x_.y),
                            floorf(j.z - 7.0f * x_.z), floorf(j.w - 7.0f * x_.w));

    float4 x = make_float4(x_.x * ns.x + ns.y, x_.y * ns.x + ns.y, x_.z * ns.x + ns.y, x_.w * ns.x + ns.y);
    float4 y = make_float4(y_.x * ns.x + ns.y, y_.y * ns.x + ns.y, y_.z * ns.x + ns.y, y_.w * ns.x + ns.y);
    float4 h = make_float4(1.0f - fabsf(x.x) - fabsf(y.x), 1.0f - fabsf(x.y) - fabsf(y.y),
                           1.0f - fabsf(x.z) - fabsf(y.z), 1.0f - fabsf(x.w) - fabsf(y.w));

    float4 b0 = make_float4(x.x, x.y, y.x, y.y);
    float4 b1 = make_float4(x.z, x.w, y.z, y.w);

    float4 s0 = make_float4(floorf(b0.x) * 2.0f + 1.0f, floorf(b0.y) * 2.0f + 1.0f,
                            floorf(b0.z) * 2.0f + 1.0f, floorf(b0.w) * 2.0f + 1.0f);
    float4 s1 = make_float4(floorf(b1.x) * 2.0f + 1.0f, floorf(b1.y) * 2.0f + 1.0f,
                            floorf(b1.z) * 2.0f + 1.0f, floorf(b1.w) * 2.0f + 1.0f);
    // sh = -step(h, 0) -> -1 si h<=0, else 0
    float4 sh = make_float4(h.x <= 0.0f ? -1.0f : 0.0f, h.y <= 0.0f ? -1.0f : 0.0f,
                            h.z <= 0.0f ? -1.0f : 0.0f, h.w <= 0.0f ? -1.0f : 0.0f);

    // a0 = b0.xzyw + s0.xzyw * sh.xxyy
    float4 a0 = make_float4(b0.x + s0.x * sh.x, b0.z + s0.z * sh.x,
                            b0.y + s0.y * sh.y, b0.w + s0.w * sh.y);
    // a1 = b1.xzyw + s1.xzyw * sh.zzww
    float4 a1 = make_float4(b1.x + s1.x * sh.z, b1.z + s1.z * sh.z,
                            b1.y + s1.y * sh.w, b1.w + s1.w * sh.w);

    float3 p0 = make_float3(a0.x, a0.y, h.x);
    float3 p1 = make_float3(a0.z, a0.w, h.y);
    float3 p2 = make_float3(a1.x, a1.y, h.z);
    float3 p3 = make_float3(a1.z, a1.w, h.w);

    float4 norm = taylorInvSqrt4(make_float4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 = p0 * norm.x; p1 = p1 * norm.y; p2 = p2 * norm.z; p3 = p3 * norm.w;

    float4 m = make_float4(fmaxf(0.6f - dot(x0, x0), 0.0f), fmaxf(0.6f - dot(x1, x1), 0.0f),
                           fmaxf(0.6f - dot(x2, x2), 0.0f), fmaxf(0.6f - dot(x3, x3), 0.0f));
    m = m * m;
    float4 m2 = m * m;
    return 42.0f * (m2.x * dot(p0, x0) + m2.y * dot(p1, x1) + m2.z * dot(p2, x2) + m2.w * dot(p3, x3));
}

// ---------------------------------------------------------------------------
// grad4 (auxiliar para 4D)
// ---------------------------------------------------------------------------
__device__ inline float4 grad4(float j, float4 ip) {
    // p.xyz = floor(fract(vec3(j)*ip.xyz)*7)*ip.z - 1
    float3 pj = make_float3(floorf(fractf(j * ip.x) * 7.0f) * ip.z - 1.0f,
                            floorf(fractf(j * ip.y) * 7.0f) * ip.z - 1.0f,
                            floorf(fractf(j * ip.z) * 7.0f) * ip.z - 1.0f);
    float pw = 1.5f - (fabsf(pj.x) + fabsf(pj.y) + fabsf(pj.z));
    float4 p = make_float4(pj.x, pj.y, pj.z, pw);
    // s = lessThan(p, 0)
    float4 s = make_float4(p.x < 0.0f ? 1.0f : 0.0f, p.y < 0.0f ? 1.0f : 0.0f,
                           p.z < 0.0f ? 1.0f : 0.0f, p.w < 0.0f ? 1.0f : 0.0f);
    // p.xyz = p.xyz + (s.xyz*2-1)*s.w
    p.x = p.x + (s.x * 2.0f - 1.0f) * s.w;
    p.y = p.y + (s.y * 2.0f - 1.0f) * s.w;
    p.z = p.z + (s.z * 2.0f - 1.0f) * s.w;
    return p;
}

// ---------------------------------------------------------------------------
// Simplex Noise 4D
// ---------------------------------------------------------------------------
__device__ inline float snoise4(float4 v) {
    const float Cx = 0.138196601125011f;
    const float Cy = 0.276393202250021f;
    const float Cz = 0.414589803375032f;
    const float Cw = -0.447213595499958f;
    const float F4 = 0.309016994374947f;

    float s = (v.x + v.y + v.z + v.w) * F4;
    float4 i = make_float4(floorf(v.x + s), floorf(v.y + s), floorf(v.z + s), floorf(v.w + s));
    float si = (i.x + i.y + i.z + i.w) * Cx;
    float4 x0 = make_float4(v.x - i.x + si, v.y - i.y + si, v.z - i.z + si, v.w - i.w + si);

    // isX = step(x0.yzw, x0.xxx); isYZ = step(x0.zww, x0.yyz)
    float3 isX = make_float3(x0.x >= x0.y ? 1.0f : 0.0f, x0.x >= x0.z ? 1.0f : 0.0f, x0.x >= x0.w ? 1.0f : 0.0f);
    float3 isYZ = make_float3(x0.y >= x0.z ? 1.0f : 0.0f, x0.y >= x0.w ? 1.0f : 0.0f, x0.z >= x0.w ? 1.0f : 0.0f);

    float4 i0;
    i0.x = isX.x + isX.y + isX.z;
    i0.y = 1.0f - isX.x;
    i0.z = 1.0f - isX.y;
    i0.w = 1.0f - isX.z;
    i0.y += isYZ.x + isYZ.y;
    i0.z += 1.0f - isYZ.x;
    i0.w += 1.0f - isYZ.y;
    i0.z += isYZ.z;
    i0.w += 1.0f - isYZ.z;

    float4 i3 = make_float4(clampf(i0.x, 0.0f, 1.0f), clampf(i0.y, 0.0f, 1.0f), clampf(i0.z, 0.0f, 1.0f), clampf(i0.w, 0.0f, 1.0f));
    float4 i2 = make_float4(clampf(i0.x - 1.0f, 0.0f, 1.0f), clampf(i0.y - 1.0f, 0.0f, 1.0f), clampf(i0.z - 1.0f, 0.0f, 1.0f), clampf(i0.w - 1.0f, 0.0f, 1.0f));
    float4 i1 = make_float4(clampf(i0.x - 2.0f, 0.0f, 1.0f), clampf(i0.y - 2.0f, 0.0f, 1.0f), clampf(i0.z - 2.0f, 0.0f, 1.0f), clampf(i0.w - 2.0f, 0.0f, 1.0f));

    float4 x1 = make_float4(x0.x - i1.x + Cx, x0.y - i1.y + Cx, x0.z - i1.z + Cx, x0.w - i1.w + Cx);
    float4 x2 = make_float4(x0.x - i2.x + Cy, x0.y - i2.y + Cy, x0.z - i2.z + Cy, x0.w - i2.w + Cy);
    float4 x3 = make_float4(x0.x - i3.x + Cz, x0.y - i3.y + Cz, x0.z - i3.z + Cz, x0.w - i3.w + Cz);
    float4 x4 = make_float4(x0.x + Cw, x0.y + Cw, x0.z + Cw, x0.w + Cw);

    i = mod289_4(i);
    float j0 = permute1(permute1(permute1(permute1(i.w) + i.z) + i.y) + i.x);
    float4 j1 = permute4(permute4(permute4(permute4(
        make_float4(i.w + i1.w, i.w + i2.w, i.w + i3.w, i.w + 1.0f))
        + make_float4(i.z + i1.z, i.z + i2.z, i.z + i3.z, i.z + 1.0f))
        + make_float4(i.y + i1.y, i.y + i2.y, i.y + i3.y, i.y + 1.0f))
        + make_float4(i.x + i1.x, i.x + i2.x, i.x + i3.x, i.x + 1.0f));

    float4 ip = make_float4(1.0f / 294.0f, 1.0f / 49.0f, 1.0f / 7.0f, 0.0f);

    float4 p0 = grad4(j0, ip);
    float4 p1 = grad4(j1.x, ip);
    float4 p2 = grad4(j1.y, ip);
    float4 p3 = grad4(j1.z, ip);
    float4 p4 = grad4(j1.w, ip);

    float4 norm = taylorInvSqrt4(make_float4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 = p0 * norm.x; p1 = p1 * norm.y; p2 = p2 * norm.z; p3 = p3 * norm.w;
    p4 = p4 * taylorInvSqrt1(dot(p4, p4));

    float3 m0 = make_float3(fmaxf(0.6f - dot(x0, x0), 0.0f), fmaxf(0.6f - dot(x1, x1), 0.0f), fmaxf(0.6f - dot(x2, x2), 0.0f));
    float2 m1 = make_float2(fmaxf(0.6f - dot(x3, x3), 0.0f), fmaxf(0.6f - dot(x4, x4), 0.0f));
    m0 = m0 * m0; float3 m0b = m0 * m0;
    float2 m1s = make_float2(m1.x * m1.x, m1.y * m1.y);
    float2 m1b = make_float2(m1s.x * m1s.x, m1s.y * m1s.y);

    return 49.0f * (m0b.x * dot(p0, x0) + m0b.y * dot(p1, x1) + m0b.z * dot(p2, x2)
                  + m1b.x * dot(p3, x3) + m1b.y * dot(p4, x4));
}

// ---------------------------------------------------------------------------
// snoiseVec3: 3 muestras de ruido 4D con semillas distintas
// ---------------------------------------------------------------------------
__device__ inline float3 snoiseVec3(float4 v) {
    float x = snoise4(v);
    float y = snoise4(v + make_float4(123.4f, 567.8f, 901.2f, 0.0f));
    float z = snoise4(v + make_float4(987.6f, 543.2f, 109.8f, 0.0f));
    return make_float3(x, y, z);
}
