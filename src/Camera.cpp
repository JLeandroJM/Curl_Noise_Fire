#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

// Metal NDC is z in [0, 1] (same convention as Vulkan / D3D). GLM exposes
// this through GLM_FORCE_DEPTH_ZERO_TO_ONE, but we keep the GLM defaults and
// just compose the projection manually to keep the depth range correct.
namespace {
glm::mat4 perspectiveMetal(float fovY, float aspect, float zNear, float zFar) {
    const float f = 1.0f / std::tan(fovY * 0.5f);
    glm::mat4 m(0.0f);
    m[0][0] = f / aspect;
    m[1][1] = f;
    m[2][2] = zFar / (zNear - zFar);
    m[2][3] = -1.0f;
    m[3][2] = (zNear * zFar) / (zNear - zFar);
    return m;
}
}

Camera::Camera(glm::vec3 eye, glm::vec3 target, glm::vec3 up,
               float fovYRadians, float aspect, float zNear, float zFar)
    : eye_(eye), target_(target), up_(up),
      fovY_(fovYRadians), aspect_(aspect), near_(zNear), far_(zFar) {}

glm::mat4 Camera::viewProj() const {
    glm::mat4 view = glm::lookAt(eye_, target_, up_);
    glm::mat4 proj = perspectiveMetal(fovY_, aspect_, near_, far_);
    return proj * view;
}

glm::vec3 Camera::cameraRight() const {
    glm::vec3 fwd = glm::normalize(target_ - eye_);
    return glm::normalize(glm::cross(fwd, up_));
}

glm::vec3 Camera::cameraUp() const {
    glm::vec3 fwd = glm::normalize(target_ - eye_);
    glm::vec3 right = glm::normalize(glm::cross(fwd, up_));
    return glm::normalize(glm::cross(right, fwd));
}
