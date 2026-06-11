#pragma once
// =============================================================================
// Shader.h - Utilidad para compilar y gestionar programas GLSL
// Proyecto: Simulación de Fuego con Curl Noise
// =============================================================================

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
    Shader() = default;
    ~Shader();

    // No copiar, solo mover
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // --- Carga de shaders desde archivo ---

    // Carga un programa gráfico (vertex + fragment, opcionalmente geometry)
    bool loadGraphics(const std::string& vertPath, const std::string& fragPath,
                      const std::string& geomPath = "");

    // Carga un compute shader
    bool loadCompute(const std::string& compPath);

    // --- Uso ---
    void use() const;
    GLuint getID() const { return programID; }

    // --- Uniforms ---
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;
    void setBool(const std::string& name, bool value) const;
    void setUint(const std::string& name, uint32_t value) const;

private:
    GLuint programID = 0;
    mutable std::unordered_map<std::string, GLint> uniformCache;

    GLint getUniformLocation(const std::string& name) const;
    std::string readFile(const std::string& path) const;
    std::string processIncludes(const std::string& source, const std::string& directory) const;
    GLuint compileShader(GLenum type, const std::string& source, const std::string& path) const;
    bool linkProgram(GLuint program) const;
    void cleanup();
};
