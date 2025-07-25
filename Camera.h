#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

class Camera {
public:
    Camera(glm::vec3 pos, glm::vec3 target, glm::vec3 up, float movSpeed, float mouseSens);

    glm::mat4 GetViewMatrix();
    glm::mat4 GetProjectionMatrix(float aspectRatio);

    void ProcessKeyboard(Camera_Movement dir, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset);

private:
    glm::vec3 positionVec;
    glm::vec3 targetVec;
    glm::vec3 upVec;

    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float MovementSpeed;
    float MouseSensitivity;

	void   updateOrientation(); // helper function to update camera orientation based on yaw and pitch
};

#endif
