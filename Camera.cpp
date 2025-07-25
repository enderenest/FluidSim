#include "Camera.h"

Camera::Camera(glm::vec3 pos, glm::vec3 target, glm::vec3 up, float movSpeed, float mouseSens)
    : positionVec(pos),
    targetVec(target),
    upVec(up),
    MovementSpeed(movSpeed),
    MouseSensitivity(mouseSens)
{
    front = glm::normalize(target - positionVec);
    worldUp = upVec;
    yaw = glm::degrees(atan2(front.z, front.x));
    pitch = glm::degrees(asin(front.y));
    updateOrientation();
}

void Camera::updateOrientation() {
    float y = glm::radians(yaw), p = glm::radians(pitch);
    glm::vec3 f;
    f.x = cos(y) * cos(p);
    f.y = sin(p);
    f.z = sin(y) * cos(p);
    front = glm::normalize(f);

    right = glm::normalize(glm::cross(front, worldUp));
    upVec = glm::normalize(glm::cross(right, front));

    targetVec = positionVec + front;
}

glm::mat4 Camera::GetViewMatrix() {
    return glm::lookAt(positionVec, targetVec, upVec);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) {
    return glm::perspective(glm::radians(1.f), aspectRatio, 0.01f, 100.0f);
}

void Camera::ProcessKeyboard(Camera_Movement dir, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    if (dir == FORWARD)  positionVec += front * velocity;
    if (dir == BACKWARD) positionVec -= front * velocity;
    if (dir == LEFT)     positionVec -= right * velocity;
    if (dir == RIGHT)    positionVec += right * velocity;
    if (dir == UP)       positionVec += worldUp * velocity;
    if (dir == DOWN)     positionVec -= worldUp * velocity;
    updateOrientation();
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateOrientation();
}
