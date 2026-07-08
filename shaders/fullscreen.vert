#version 430 core

out vec2 TexCoords;

void main() {
    // Técnica de Fullscreen Triangle sin necesidad de un VBO externo
    // Se requieren exactamente 3 vértices (glDrawArrays(GL_TRIANGLES, 0, 3))
    // 
    // Índice:
    // 0 -> x = -1, y = -1, UV = (0,0)
    // 1 -> x =  3, y = -1, UV = (2,0)
    // 2 -> x = -1, y =  3, UV = (0,2)
    //
    // El triángulo cubre completamente el espacio [-1, 1] de clip coordinates.
    
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    
    TexCoords = vec2((x + 1.0) * 0.5, (y + 1.0) * 0.5);
    gl_Position = vec4(x, y, 0.0, 1.0);
}
