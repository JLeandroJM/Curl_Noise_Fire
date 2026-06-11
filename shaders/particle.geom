#version 430 core

// Entrada de puntos del vertex shader
layout(points) in;
// Salida de quads (triangle_strip de 4 vértices)
layout(triangle_strip, max_vertices = 4) out;

in VertexData {
    vec4 color;
    float size;
    uint type;
} gs_in[];

// Salida hacia el fragment shader
out FragData {
    vec2 uv;
    vec4 color;
    flat uint type;
} gs_out;

// Uniforms de matrices de transformación
uniform mat4 view;
uniform mat4 projection;

// Vectores para alineación a la cámara (billboarding)
uniform vec3 cameraRight;
uniform vec3 cameraUp;

void main() {
    // Si la partícula está muerta, descartarla (no emitir vértices)
    if (gs_in[0].type == 0u) {
        return;
    }

    vec3 pos = gl_in[0].gl_Position.xyz;
    float size = gs_in[0].size;

    // Modificadores de tamaño visuales específicos según tipo
    if (gs_in[0].type == 5u) { // SPARK (Chispa)
        size *= 0.3; // Más pequeñas
    } else if (gs_in[0].type >= 10u) { // MATERIALES ESTÁTICOS
        size *= 0.5;
    }

    // Calcular ejes del quad multiplicados por el tamaño
    vec3 right = cameraRight * size;
    vec3 up = cameraUp * size;

    // Generar el quad con coordenadas UV de 0.0 a 1.0

    // Vértice 1: Superior Derecho
    gl_Position = projection * view * vec4(pos + right + up, 1.0);
    gs_out.uv = vec2(1.0, 1.0);
    gs_out.color = gs_in[0].color;
    gs_out.type = gs_in[0].type;
    EmitVertex();

    // Vértice 2: Superior Izquierdo
    gl_Position = projection * view * vec4(pos - right + up, 1.0);
    gs_out.uv = vec2(0.0, 1.0);
    gs_out.color = gs_in[0].color;
    gs_out.type = gs_in[0].type;
    EmitVertex();

    // Vértice 3: Inferior Derecho
    gl_Position = projection * view * vec4(pos + right - up, 1.0);
    gs_out.uv = vec2(1.0, 0.0);
    gs_out.color = gs_in[0].color;
    gs_out.type = gs_in[0].type;
    EmitVertex();

    // Vértice 4: Inferior Izquierdo
    gl_Position = projection * view * vec4(pos - right - up, 1.0);
    gs_out.uv = vec2(0.0, 0.0);
    gs_out.color = gs_in[0].color;
    gs_out.type = gs_in[0].type;
    EmitVertex();

    EndPrimitive();
}
