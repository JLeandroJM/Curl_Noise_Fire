#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

Shader::~Shader() {
    cleanup();
}

Shader::Shader(Shader&& other) noexcept : programID(other.programID), uniformCache(std::move(other.uniformCache)) {
    other.programID = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        cleanup();
        programID = other.programID;
        uniformCache = std::move(other.uniformCache);
        other.programID = 0;
    }
    return *this;
}

void Shader::cleanup() {
    if (programID != 0) {
        glDeleteProgram(programID);
        programID = 0;
    }
}

std::string Shader::readFile(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de shader: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string Shader::processIncludes(const std::string& source, const std::string& directory) const {
    std::istringstream stream(source);
    std::string line;
    std::stringstream output;

    while (std::getline(stream, line)) {
        if (line.find("#include") != std::string::npos) {
            size_t start = line.find("\"");
            size_t end = line.rfind("\"");
            if (start != std::string::npos && end != std::string::npos && start < end) {
                std::string includeFile = line.substr(start + 1, end - start - 1);
                std::string includePath = directory + "/" + includeFile;
                std::string includeSource = readFile(includePath);
                // Recursivamente procesar includes
                output << processIncludes(includeSource, directory) << "\n";
            }
        } else {
            output << line << "\n";
        }
    }
    return output.str();
}

GLuint Shader::compileShader(GLenum type, const std::string& source, const std::string& path) const {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::string typeStr;
        switch (type) {
            case GL_VERTEX_SHADER: typeStr = "VERTEX"; break;
            case GL_FRAGMENT_SHADER: typeStr = "FRAGMENT"; break;
            case GL_GEOMETRY_SHADER: typeStr = "GEOMETRY"; break;
            case GL_COMPUTE_SHADER: typeStr = "COMPUTE"; break;
            default: typeStr = "UNKNOWN"; break;
        }
        std::cerr << "Error compiling " << typeStr << " shader (" << path << "):\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Shader::linkProgram(GLuint program) const {
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        std::cerr << "Error linking shader program:\n" << infoLog << std::endl;
        return false;
    }
    return true;
}

bool Shader::loadGraphics(const std::string& vertPath, const std::string& fragPath, const std::string& geomPath) {
    cleanup();
    
    std::filesystem::path vp(vertPath);
    std::string directory = vp.parent_path().string();

    std::string vertSource = processIncludes(readFile(vertPath), directory);
    std::string fragSource = processIncludes(readFile(fragPath), directory);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertSource, vertPath);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragSource, fragPath);
    GLuint geometryShader = 0;

    if (!vertSource.empty() && vertexShader == 0) return false;
    if (!fragSource.empty() && fragmentShader == 0) return false;

    if (!geomPath.empty()) {
        std::string geomSource = processIncludes(readFile(geomPath), directory);
        geometryShader = compileShader(GL_GEOMETRY_SHADER, geomSource, geomPath);
        if (geometryShader == 0) return false;
    }

    programID = glCreateProgram();
    if (vertexShader) glAttachShader(programID, vertexShader);
    if (fragmentShader) glAttachShader(programID, fragmentShader);
    if (geometryShader) glAttachShader(programID, geometryShader);

    bool success = linkProgram(programID);

    if (vertexShader) glDeleteShader(vertexShader);
    if (fragmentShader) glDeleteShader(fragmentShader);
    if (geometryShader) glDeleteShader(geometryShader);

    uniformCache.clear();
    return success;
}

bool Shader::loadCompute(const std::string& compPath) {
    cleanup();

    std::filesystem::path cp(compPath);
    std::string directory = cp.parent_path().string();

    std::string compSource = processIncludes(readFile(compPath), directory);
    GLuint computeShader = compileShader(GL_COMPUTE_SHADER, compSource, compPath);

    if (computeShader == 0) return false;

    programID = glCreateProgram();
    glAttachShader(programID, computeShader);
    bool success = linkProgram(programID);

    glDeleteShader(computeShader);
    uniformCache.clear();
    return success;
}

void Shader::use() const {
    glUseProgram(programID);
}

GLint Shader::getUniformLocation(const std::string& name) const {
    auto it = uniformCache.find(name);
    if (it != uniformCache.end()) {
        return it->second;
    }
    GLint location = glGetUniformLocation(programID, name.c_str());
    uniformCache[name] = location;
    return location;
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(getUniformLocation(name), (int)value);
}

void Shader::setUint(const std::string& name, uint32_t value) const {
    glUniform1ui(getUniformLocation(name), value);
}
