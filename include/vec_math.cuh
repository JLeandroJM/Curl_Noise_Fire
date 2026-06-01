// =============================================================================
// vec_math.cuh - Operadores vectoriales mínimos para float2/float3/float4
// Disponibles tanto en host como en device (__host__ __device__).
// Se usan en el port CUDA del simplex/curl noise y en el rasterizador.
// =============================================================================
#pragma once

#include <cuda_runtime.h>
#include <math.h>

// ---------------------------------------------------------------------------
// Constructores cortos
// ---------------------------------------------------------------------------
__host__ __device__ inline float3 mkf3(float x, float y, float z) { return make_float3(x, y, z); }
__host__ __device__ inline float4 mkf4(float x, float y, float z, float w) { return make_float4(x, y, z, w); }
__host__ __device__ inline float3 mkf3(float s) { return make_float3(s, s, s); }
__host__ __device__ inline float4 mkf4(float s) { return make_float4(s, s, s, s); }

// ---------------------------------------------------------------------------
// float3
// ---------------------------------------------------------------------------
__host__ __device__ inline float3 operator+(float3 a, float3 b) { return make_float3(a.x + b.x, a.y + b.y, a.z + b.z); }
__host__ __device__ inline float3 operator-(float3 a, float3 b) { return make_float3(a.x - b.x, a.y - b.y, a.z - b.z); }
__host__ __device__ inline float3 operator*(float3 a, float3 b) { return make_float3(a.x * b.x, a.y * b.y, a.z * b.z); }
__host__ __device__ inline float3 operator*(float3 a, float s)   { return make_float3(a.x * s, a.y * s, a.z * s); }
__host__ __device__ inline float3 operator*(float s, float3 a)   { return make_float3(a.x * s, a.y * s, a.z * s); }
__host__ __device__ inline float3 operator/(float3 a, float s)   { return make_float3(a.x / s, a.y / s, a.z / s); }
__host__ __device__ inline float3 operator-(float3 a)            { return make_float3(-a.x, -a.y, -a.z); }
__host__ __device__ inline void   operator+=(float3& a, float3 b) { a.x += b.x; a.y += b.y; a.z += b.z; }

__host__ __device__ inline float dot(float3 a, float3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
__host__ __device__ inline float3 cross(float3 a, float3 b) {
    return make_float3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
__host__ __device__ inline float  length(float3 a) { return sqrtf(dot(a, a)); }
__host__ __device__ inline float3 normalize(float3 a) {
    float l = length(a);
    return l > 1e-8f ? a / l : make_float3(0.0f, 0.0f, 0.0f);
}
__host__ __device__ inline float3 floor3(float3 a) { return make_float3(floorf(a.x), floorf(a.y), floorf(a.z)); }
__host__ __device__ inline float3 abs3(float3 a)   { return make_float3(fabsf(a.x), fabsf(a.y), fabsf(a.z)); }
__host__ __device__ inline float3 fmaxf3(float3 a, float s) { return make_float3(fmaxf(a.x, s), fmaxf(a.y, s), fmaxf(a.z, s)); }
__host__ __device__ inline float3 step3(float3 edge, float3 x) {
    return make_float3(x.x >= edge.x ? 1.0f : 0.0f, x.y >= edge.y ? 1.0f : 0.0f, x.z >= edge.z ? 1.0f : 0.0f);
}

// ---------------------------------------------------------------------------
// float4
// ---------------------------------------------------------------------------
__host__ __device__ inline float4 operator+(float4 a, float4 b) { return make_float4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
__host__ __device__ inline float4 operator-(float4 a, float4 b) { return make_float4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
__host__ __device__ inline float4 operator*(float4 a, float4 b) { return make_float4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }
__host__ __device__ inline float4 operator*(float4 a, float s)   { return make_float4(a.x * s, a.y * s, a.z * s, a.w * s); }
__host__ __device__ inline float4 operator*(float s, float4 a)   { return make_float4(a.x * s, a.y * s, a.z * s, a.w * s); }
__host__ __device__ inline float4 operator-(float4 a, float s)   { return make_float4(a.x - s, a.y - s, a.z - s, a.w - s); }
__host__ __device__ inline float4 operator+(float4 a, float s)   { return make_float4(a.x + s, a.y + s, a.z + s, a.w + s); }
__host__ __device__ inline float4 operator-(float4 a)            { return make_float4(-a.x, -a.y, -a.z, -a.w); }

__host__ __device__ inline float dot(float4 a, float4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
__host__ __device__ inline float4 floor4(float4 a) { return make_float4(floorf(a.x), floorf(a.y), floorf(a.z), floorf(a.w)); }
__host__ __device__ inline float4 abs4(float4 a)   { return make_float4(fabsf(a.x), fabsf(a.y), fabsf(a.z), fabsf(a.w)); }
__host__ __device__ inline float4 max4(float4 a, float s) { return make_float4(fmaxf(a.x, s), fmaxf(a.y, s), fmaxf(a.z, s), fmaxf(a.w, s)); }
__host__ __device__ inline float4 min4(float4 a, float s) { return make_float4(fminf(a.x, s), fminf(a.y, s), fminf(a.z, s), fminf(a.w, s)); }

// step(edge, x): 1.0 si x >= edge
__host__ __device__ inline float4 step4(float4 edge, float4 x) {
    return make_float4(x.x >= edge.x ? 1.0f : 0.0f,
                       x.y >= edge.y ? 1.0f : 0.0f,
                       x.z >= edge.z ? 1.0f : 0.0f,
                       x.w >= edge.w ? 1.0f : 0.0f);
}
// lessThan(a, b): 1.0 si a < b (como vec4 de bool en GLSL)
__host__ __device__ inline float4 lessThan4(float4 a, float4 b) {
    return make_float4(a.x < b.x ? 1.0f : 0.0f,
                       a.y < b.y ? 1.0f : 0.0f,
                       a.z < b.z ? 1.0f : 0.0f,
                       a.w < b.w ? 1.0f : 0.0f);
}

// ---------------------------------------------------------------------------
// Escalares utilitarios
// ---------------------------------------------------------------------------
__host__ __device__ inline float clampf(float x, float lo, float hi) { return fminf(fmaxf(x, lo), hi); }
__host__ __device__ inline float fractf(float x) { return x - floorf(x); }
__host__ __device__ inline float mixf(float a, float b, float t) { return a + (b - a) * t; }
__host__ __device__ inline float3 mix3(float3 a, float3 b, float t) { return a + (b - a) * t; }
__host__ __device__ inline float4 mix4(float4 a, float4 b, float t) { return a + (b - a) * t; }
__host__ __device__ inline float smoothstepf(float e0, float e1, float x) {
    float t = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
