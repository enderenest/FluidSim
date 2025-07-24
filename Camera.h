#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(glm::vec3 pos, glm::vec3 target, glm::vec3 up) {
        this -> position = pos;
        this -> target = target;
		this -> up = up;
    }
    glm::mat4 GetViewMatrix();
    glm::mat4 GetProjectionMatrix(float aspectRatio);

private:
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
};

#endif
