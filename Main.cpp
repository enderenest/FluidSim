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

const unsigned int WIDTH = 800, HEIGHT = 800;
const unsigned int PARTICLE_COUNT = 200;
const unsigned int CIRCLE_SEGMENTS = 16;
const float PARTICLE_RADIUS = 0.1f;
const float MASS = 1.0f;
const float GRAVITY = 0.0f;
const float COLLISION_DAMPING = 1.0f;
const float BOUNDARY_X = 0.9f;
const float BOUNDARY_Y = 0.9f;
const float BOUNDARY_Z = 0.9f;
const float SPACING = 0.03f;
const float SMOOTHING_RADIUS = 0.2f;
const float PRESSURE_MULTIPLIER = 10.0f;
const float TARGET_DENSITY = 30.0f;


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


int main() 
{
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

    Fluid fluid(PARTICLE_COUNT, MASS, GRAVITY, COLLISION_DAMPING, SPACING, PRESSURE_MULTIPLIER, TARGET_DENSITY, SMOOTHING_RADIUS);

    std::vector<glm::vec3> circleVertices;
    std::vector<GLuint> circleIndices;
    CreateUnitCircle(circleVertices, circleIndices, CIRCLE_SEGMENTS, PARTICLE_RADIUS);

    VAO vao1;
	vao1.Bind();

    VBO vboCircle(circleVertices.data(), circleVertices.size() * sizeof(glm::vec3));
    vao1.LinkVBO(vboCircle, 0);

	EBO ebo1(circleIndices.data(), circleIndices.size() * sizeof(GLuint));

    std::vector<glm::vec3> instancePositions(PARTICLE_COUNT);
    VBO instanceVBO(instancePositions.data(), instancePositions.size() * sizeof(glm::vec3));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(1, 1);

    vao1.Unbind();
	vboCircle.Unbind();
	instanceVBO.Unbind();
	ebo1.Unbind();

    // Time
    float lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Simulate
        fluid.Update(deltaTime);
        fluid.HandleBoundaryCollisions(BOUNDARY_X, BOUNDARY_Y, BOUNDARY_Z);
		fluid.GetParticlePositions(instancePositions);

        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO.ID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, instancePositions.size() * sizeof(glm::vec3), instancePositions.data());

        // Draw
        shaderProgram.Activate();
		vao1.Bind();
        glUniform1f(glGetUniformLocation(shaderProgram.ID, "radius"), PARTICLE_RADIUS);
        glDrawElementsInstanced(GL_TRIANGLES, circleIndices.size(), GL_UNSIGNED_INT, 0, PARTICLE_COUNT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    vao1.Delete();
    vboCircle.Delete();
    instanceVBO.Delete();
    ebo1.Delete();
    shaderProgram.Delete();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

