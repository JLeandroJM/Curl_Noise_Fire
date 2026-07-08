#pragma once
// =============================================================================
// Camera.h - Cámara con rutas de animación pre-definidas
// Proyecto: Simulación de Fuego con Curl Noise
// =============================================================================

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Particle.h" // Para CameraPath y CameraKeyframe
#include <vector>

class Camera {
public:
    Camera();

    // Configurar la ruta de animación de la cámara
    void setPath(const CameraPath& path);

    // Actualizar posición de la cámara según el tiempo actual
    void update(float currentTime);

    // Obtener matrices
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getUp() const { return up; }
    glm::vec3 getRight() const;

    // Configuración
    void setFOV(float fov) { this->fov = fov; }
    void setNearFar(float near, float far) { nearPlane = near; farPlane = far; }

private:
    glm::vec3 position = glm::vec3(0.0f, 2.0f, 5.0f);
    glm::vec3 target   = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f);

    float fov       = 45.0f;
    float nearPlane = 0.1f;
    float farPlane  = 100.0f;

    CameraPath currentPath;

    // Interpola entre dos keyframes
    void interpolateKeyframes(const CameraKeyframe& a, const CameraKeyframe& b, float t);
};
