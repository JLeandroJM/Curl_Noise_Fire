#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    Camera(glm::vec3 eye, glm::vec3 target, glm::vec3 up,
           float fovYRadians, float aspect, float zNear, float zFar);

    void setAspect(float aspect) { aspect_ = aspect; }

    glm::mat4 viewProj() const;     // column-major, Metal-friendly
    glm::vec3 cameraRight() const;  // world-space right of view basis
    glm::vec3 cameraUp() const;     // world-space up of view basis

private:
    glm::vec3 eye_;
    glm::vec3 target_;
    glm::vec3 up_;
    float fovY_;
    float aspect_;
    float near_;
    float far_;
};
