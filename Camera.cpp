#include "Camera.h"

glm::mat4 Camera::GetViewMatrix() {
    return glm::lookAt(position, target, up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) {
    return glm::perspective(glm::radians(1.f), aspectRatio, 0.01f, 100.0f);
}
