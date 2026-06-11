#include "Camera.h"
#include <glm/gtx/compatibility.hpp> // For lerp
#include <algorithm>

Camera::Camera() {
    // Default keyframe if path is empty
    currentPath.keyframes.push_back({0.0f, position, target});
}

void Camera::setPath(const CameraPath& path) {
    currentPath = path;
    if (!currentPath.keyframes.empty()) {
        position = currentPath.keyframes.front().position;
        target = currentPath.keyframes.front().target;
    }
}

void Camera::update(float currentTime) {
    if (currentPath.keyframes.empty()) return;

    // Animación cíclica
    float t = std::fmod(currentTime, currentPath.totalDuration);
    if (t < 0.0f) t += currentPath.totalDuration;

    // Encontrar los keyframes entre los cuales interpolar
    auto it = std::lower_bound(currentPath.keyframes.begin(), currentPath.keyframes.end(), t,
        [](const CameraKeyframe& kf, float time) {
            return kf.time < time;
        });

    if (it == currentPath.keyframes.end()) {
        // Después del último keyframe (no debería pasar con fmod si max time == totalDuration, pero por si acaso)
        position = currentPath.keyframes.back().position;
        target = currentPath.keyframes.back().target;
    } else if (it == currentPath.keyframes.begin()) {
        // Antes del primer keyframe
        position = currentPath.keyframes.front().position;
        target = currentPath.keyframes.front().target;
    } else {
        auto prev = it - 1;
        auto next = it;
        
        float segmentDuration = next->time - prev->time;
        float segmentTime = t - prev->time;
        float factor = segmentDuration > 0.0f ? segmentTime / segmentDuration : 0.0f;

        interpolateKeyframes(*prev, *next, factor);
    }
}

void Camera::interpolateKeyframes(const CameraKeyframe& a, const CameraKeyframe& b, float t) {
    // Suavizado en los extremos (ease-in-out simple)
    float smoothT = t * t * (3.0f - 2.0f * t);
    
    position = glm::lerp(a.position, b.position, smoothT);
    target = glm::lerp(a.target, b.target, smoothT);
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, target, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

glm::vec3 Camera::getRight() const {
    glm::vec3 forward = glm::normalize(target - position);
    return glm::normalize(glm::cross(forward, up));
}
