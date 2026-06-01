// =============================================================================
// CurlNoise.cuh - Port a CUDA de shaders/curl_noise.glsl
// Campo vectorial sin divergencia: v = ∇ × Ψ, calculado con diferencias finitas
// centrales sobre simplex noise 4D (3 espacio + 1 tiempo).
// =============================================================================
#pragma once

#include "SimplexNoise.cuh"

// Parámetros del curl noise (espejo de CurlNoiseParams en el host).
struct GpuCurlParams {
    float frequency;
    float amplitude;
    int   octaves;
    float lacunarity;
    float persistence;
    float timeScale;
    float epsilon;
};

// Curl de una sola octava usando diferencias finitas centrales.
__device__ inline float3 computeCurl(float3 p, float t, float e) {
    float4 p0 = make_float4(p.x, p.y, p.z, t);

    float4 dx = make_float4(e, 0.0f, 0.0f, 0.0f);
    float4 dy = make_float4(0.0f, e, 0.0f, 0.0f);
    float4 dz = make_float4(0.0f, 0.0f, e, 0.0f);

    // x = d(psi_z)/dy - d(psi_y)/dz
    float p_y0 = snoiseVec3(p0 - dy).z;
    float p_y1 = snoiseVec3(p0 + dy).z;
    float p_z0 = snoiseVec3(p0 - dz).y;
    float p_z1 = snoiseVec3(p0 + dz).y;
    float x = (p_y1 - p_y0) - (p_z1 - p_z0);

    // y = d(psi_x)/dz - d(psi_z)/dx
    float p_z2 = snoiseVec3(p0 - dz).x;
    float p_z3 = snoiseVec3(p0 + dz).x;
    float p_x0 = snoiseVec3(p0 - dx).z;
    float p_x1 = snoiseVec3(p0 + dx).z;
    float y = (p_z3 - p_z2) - (p_x1 - p_x0);

    // z = d(psi_y)/dx - d(psi_x)/dy
    float p_x2 = snoiseVec3(p0 - dx).y;
    float p_x3 = snoiseVec3(p0 + dx).y;
    float p_y2 = snoiseVec3(p0 - dy).x;
    float p_y3 = snoiseVec3(p0 + dy).x;
    float z = (p_x3 - p_x2) - (p_y3 - p_y2);

    return make_float3(x, y, z) / (2.0f * e);
}

// Curl noise multi-octava (fBm sobre el campo de curl).
__device__ inline float3 curlNoise(float3 p, float t, const GpuCurlParams& cp) {
    float3 curl = make_float3(0.0f, 0.0f, 0.0f);
    float freq = cp.frequency;
    float amp = cp.amplitude;
    float time = t * cp.timeScale;

    int octaves = cp.octaves > 0 ? cp.octaves : 1;
    for (int i = 0; i < octaves; ++i) {
        curl += computeCurl(p * freq, time, cp.epsilon) * amp;
        freq *= cp.lacunarity;
        amp *= cp.persistence;
    }
    return curl;
}
