#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Fluid.h"
#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"

#include <vector>
#include <cmath>

// influence = SmmothingKernel(smmothingRadius, distance)
// density += influence * mass;
// slope = SmoothingKernelDerivative(smoothingRadius, distance)
// float densityError = density - _targetDensity;
// float pressure = _pressureMultiplier * densityError;
// pressureForce = pressure * slope * direction * mass / density;
// pressureAcceleration = pressureForce / density;
// velocity += pressureAcceleration * dt;


const unsigned int WIDTH = 800, HEIGHT = 800;
const unsigned int PARTICLE_COUNT = 1500;
const unsigned int CIRCLE_SEGMENTS = 8;
const unsigned int SPATIAL_HASH_SIZE = 4096;
const float PARTICLE_RADIUS = 0.08f;
const float MASS = 0.08f;
const float GRAVITY = 3.0f;
const float COLLISION_DAMPING = 0.35f;
const float BOUNDARY_X = 0.9f;
const float BOUNDARY_Y = 0.9f;
const float BOUNDARY_Z = 0.9f;
const float SPACING = 0.02f;
const float SMOOTHING_RADIUS = 0.1f;
const float PRESSURE_MULTIPLIER = 15.0f;
const float TARGET_DENSITY = 80.0f;
const float DELTA_TIME = 0.016f;

const float INTERACTION_RADIUS = 0.2f;
const float INTERACTION_STRENGTH = 2.0f;

bool upLastFrame = false;
bool downLastFrame = false;
bool oneLastFrame = false;
bool twoLastFrame = false;


void static CreateUnitCircle(std::vector<glm::vec3>& vertices, std::vector<GLuint>& indices, int segments = 16, float radius = 0.1f) {
    vertices.clear();
    indices.clear();

    float angle = 360.0f / segments;
	int triangleCount = segments - 2;

    for (int i = 0; i < segments; i++)
    {
        float currentAngle = angle * i;
        float x = radius * cos(glm::radians(currentAngle));
        float y = radius * sin(glm::radians(currentAngle));
        float z = 0.0f; // Assuming a 2D circle in the XY plane

        vertices.push_back(glm::vec3(x, y, z));
    }

    for (int i = 0; i < triangleCount; i++)
    {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
    }
}


int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Fluid Particles", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    gladLoadGL();
    glViewport(0, 0, WIDTH, HEIGHT);


    Shader shaderProgram("default.vert", "default.frag");

    Shader lineShader("line.vert", "line.frag");

	float bx = BOUNDARY_X - PARTICLE_RADIUS;
	float by = BOUNDARY_Y - PARTICLE_RADIUS;
    std::vector<glm::vec3> boundaryLines = {
        {-bx, -by, 0.0f}, { bx, -by, 0.0f},
        { bx, -by, 0.0f}, { bx,  by, 0.0f},
        { bx,  by, 0.0f}, {-bx,  by, 0.0f},
        {-bx,  by, 0.0f}, {-bx, -by, 0.0f}
    };

    GLuint boundaryVAO, boundaryVBO;
    glGenVertexArrays(1, &boundaryVAO);
    glGenBuffers(1, &boundaryVBO);

    glBindVertexArray(boundaryVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boundaryVBO);
    glBufferData(GL_ARRAY_BUFFER, boundaryLines.size() * sizeof(glm::vec3), boundaryLines.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);

    Fluid fluid(PARTICLE_COUNT, PARTICLE_RADIUS, MASS, GRAVITY, COLLISION_DAMPING, SPACING, PRESSURE_MULTIPLIER, TARGET_DENSITY, SMOOTHING_RADIUS, SPATIAL_HASH_SIZE, INTERACTION_RADIUS, INTERACTION_STRENGTH);

    std::vector<glm::vec3> circleVertices;
    std::vector<GLuint> circleIndices;
    CreateUnitCircle(circleVertices, circleIndices, CIRCLE_SEGMENTS, PARTICLE_RADIUS);

    VAO vao1;
    vao1.Bind();

    // Static circle mesh VBO
    VBO vboCircle(circleVertices.data(), circleVertices.size() * sizeof(glm::vec3));
    vao1.LinkVBO(vboCircle, 0); // layout(location = 0)

    EBO ebo1(circleIndices.data(), circleIndices.size() * sizeof(GLuint));

    // Instance data: positions
    std::vector<glm::vec3> instancePositions(PARTICLE_COUNT);
    VBO instancePosVBO(instancePositions.data(), instancePositions.size() * sizeof(glm::vec3));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(1, 1);

    // Instance data: velocities
    std::vector<glm::vec3> instanceVelocities(PARTICLE_COUNT);
    VBO instanceVelVBO(instanceVelocities.data(), instanceVelocities.size() * sizeof(glm::vec3));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(2, 1);

    vao1.Unbind();
    vboCircle.Unbind();
    instancePosVBO.Unbind();
    instanceVelVBO.Unbind();
    ebo1.Unbind();

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // ------------------ MOUSE INTERACTION -----------------------
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Convert to Normalized Device Coordinates [-1, 1]
        float x = 2.0f * static_cast<float>(mouseX) / WIDTH - 1.0f;
        float y = 1.0f - 2.0f * static_cast<float>(mouseY) / HEIGHT;
        glm::vec2 worldMousePos = glm::vec2(x, y);

        // Apply force on left-click
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            fluid.ApplyInteractionForce(worldMousePos, INTERACTION_RADIUS, INTERACTION_STRENGTH);
        }

        // Apply force on right-click
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            fluid.ApplyInteractionForce(worldMousePos, INTERACTION_RADIUS, -INTERACTION_STRENGTH);
        }
        // ------------------------------------------------------------


		// ------------------ KEYBOARD CONTROLS -----------------------
        int upState = glfwGetKey(window, GLFW_KEY_UP);
        if (upState == GLFW_PRESS && !upLastFrame) {
            fluid.SetPressureMultiplier(fluid.GetPressureMultiplier() + 1.0f);
        }
        upLastFrame = (upState == GLFW_PRESS);

        int downState = glfwGetKey(window, GLFW_KEY_DOWN);
        if (downState == GLFW_PRESS && !downLastFrame) {
            fluid.SetPressureMultiplier(fluid.GetPressureMultiplier() - 1.0f);
        }
        downLastFrame = (downState == GLFW_PRESS);

        int oneState = glfwGetKey(window, GLFW_KEY_1);
        if (oneState == GLFW_PRESS && !oneLastFrame) {
            fluid.SetTargetDensity(fluid.GetTargetDensity() + 3.0f);
        }
        oneLastFrame = (oneState == GLFW_PRESS);

        int twoState = glfwGetKey(window, GLFW_KEY_2);
        if (twoState == GLFW_PRESS && !twoLastFrame) {
            fluid.SetTargetDensity(fluid.GetTargetDensity() - 3.0f);
        }
        twoLastFrame = (twoState == GLFW_PRESS);

        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) 
                fluid.SetGravity(0.0f);

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            fluid.SetGravity(GRAVITY);

        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
			fluid.SetPaused(true);
		}

        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            fluid.SetPaused(false);
        }
		// ------------------------------------------------------------


        fluid.Update(DELTA_TIME);
        fluid.HandleBoundaryCollisions(BOUNDARY_X, BOUNDARY_Y, 0.0f);
        fluid.GetParticlePositions(instancePositions);
        fluid.GetParticleVelocities(instanceVelocities);

        glBindBuffer(GL_ARRAY_BUFFER, instancePosVBO.ID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, instancePositions.size() * sizeof(glm::vec3), instancePositions.data());

        glBindBuffer(GL_ARRAY_BUFFER, instanceVelVBO.ID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, instanceVelocities.size() * sizeof(glm::vec3), instanceVelocities.data());

        shaderProgram.Activate();
        vao1.Bind();
        glUniform1f(glGetUniformLocation(shaderProgram.ID, "scale"), PARTICLE_RADIUS);
        glDrawElementsInstanced(GL_TRIANGLES, circleIndices.size(), GL_UNSIGNED_INT, 0, PARTICLE_COUNT);

        lineShader.Activate();
        glBindVertexArray(boundaryVAO);
        glDrawArrays(GL_LINES, 0, boundaryLines.size());
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    vao1.Delete();
    vboCircle.Delete();
    instancePosVBO.Delete();
    instanceVelVBO.Delete();
    ebo1.Delete();
    shaderProgram.Delete();
    glDeleteVertexArrays(1, &boundaryVAO);
    glDeleteBuffers(1, &boundaryVBO);
    lineShader.Delete();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

