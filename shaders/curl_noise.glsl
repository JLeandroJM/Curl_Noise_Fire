// shaders/curl_noise.glsl
// Biblioteca de Curl Noise para simular dinámica de fluidos (turbulencia de fuego)

// Asume que simplex_noise.glsl ha sido incluido previamente.

// Uniforms para controlar las características del ruido curl
layout(location = 10) uniform float u_curlFrequency = 1.0;
layout(location = 11) uniform float u_curlAmplitude = 1.0;
layout(location = 12) uniform int   u_curlOctaves = 3;
layout(location = 13) uniform float u_curlLacunarity = 2.0;
layout(location = 14) uniform float u_curlPersistence = 0.5;
layout(location = 15) uniform float u_curlTimeScale = 1.0;
layout(location = 16) uniform float u_curlEpsilon = 0.001;

// Computa el curl noise de una sola octava usando diferencias finitas
// Representa la rotación (curl) de un campo de ruido vectorial tridimensional
vec3 computeCurl(vec3 p, float t) {
    float e = u_curlEpsilon;
    vec3 dx = vec3(e, 0.0, 0.0);
    vec3 dy = vec3(0.0, e, 0.0);
    vec3 dz = vec3(0.0, 0.0, e);

    vec4 p0 = vec4(p, t);
    
    // Derivada en Y de Z - Derivada en Z de Y
    float p_y0 = snoiseVec3(p0 - vec4(dy, 0.0)).z;
    float p_y1 = snoiseVec3(p0 + vec4(dy, 0.0)).z;
    float p_z0 = snoiseVec3(p0 - vec4(dz, 0.0)).y;
    float p_z1 = snoiseVec3(p0 + vec4(dz, 0.0)).y;
    float x = (p_y1 - p_y0) - (p_z1 - p_z0);

    // Derivada en Z de X - Derivada en X de Z
    float p_z2 = snoiseVec3(p0 - vec4(dz, 0.0)).x;
    float p_z3 = snoiseVec3(p0 + vec4(dz, 0.0)).x;
    float p_x0 = snoiseVec3(p0 - vec4(dx, 0.0)).z;
    float p_x1 = snoiseVec3(p0 + vec4(dx, 0.0)).z;
    float y = (p_z3 - p_z2) - (p_x1 - p_x0);

    // Derivada en X de Y - Derivada en Y de X
    float p_x2 = snoiseVec3(p0 - vec4(dx, 0.0)).y;
    float p_x3 = snoiseVec3(p0 + vec4(dx, 0.0)).y;
    float p_y2 = snoiseVec3(p0 - vec4(dy, 0.0)).x;
    float p_y3 = snoiseVec3(p0 + vec4(dy, 0.0)).x;
    float z = (p_x3 - p_x2) - (p_y3 - p_y2);

    return vec3(x, y, z) / (2.0 * e);
}

// Genera un campo de curl noise de múltiples octavas
vec3 curlNoise(vec3 p, float t) {
    vec3 curl = vec3(0.0);
    float freq = u_curlFrequency;
    float amp = u_curlAmplitude;
    float time = t * u_curlTimeScale;

    for (int i = 0; i < u_curlOctaves; ++i) {
        curl += computeCurl(p * freq, time) * amp;
        freq *= u_curlLacunarity;
        amp *= u_curlPersistence;
    }

    return curl;
}

// Curl noise que respeta fronteras (boundaries)
// Reduce la intensidad del ruido al acercarse a obstáculos usando una rampa
vec3 curlNoiseWithBoundary(vec3 p, float t, float distToObstacle) {
    // rampa de 0.0 en el obstáculo hasta 1.0 en zonas libres
    float ramp = smoothstep(0.0, 0.5, distToObstacle);
    vec3 curl = curlNoise(p, t);
    return curl * ramp;
}
