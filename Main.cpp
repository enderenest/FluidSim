#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Fluid.h"
#include"shaderClass.h"
#include"ComputeShader.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"

#include <vector>
#include <cmath>

// influence = SmoothingKernel(smoothingRadius, distance)
// density += influence * mass;
// slope = SmoothingKernelDerivative(smoothingRadius, distance)
// float densityError = density - _targetDensity;
// float pressure = _pressureMultiplier * densityError;
// pressureForce = pressure * slope * direction * mass / density;
// pressureAcceleration = pressureForce / density;
// velocity += pressureAcceleration * dt;


const unsigned int WIDTH = 900, HEIGHT = 900;
const unsigned int PARTICLE_COUNT = 1024;
const unsigned int CIRCLE_SEGMENTS = 8;
const unsigned int SPATIAL_HASH_SIZE = 4096;
const float PARTICLE_RADIUS = 0.08f;
const float MASS = 0.008f;
const float GRAVITY_ACCELERATION = 0.0f;
const float COLLISION_DAMPING = 0.3f;
const float BOUNDARY_X = 0.9f;
const float BOUNDARY_Y = 0.9f;
const float BOUNDARY_Z = 0.9f;
const float SPACING = 0.02f;
const float SMOOTHING_RADIUS = 0.1f;
const float PRESSURE_MULTIPLIER = 0.00001f;
const float TARGET_DENSITY = 80.0f;
const float VISCOSITY_STRENGTH = 0.35f;
const float NEAR_DENSITY_MULTIPLIER = 0.0f;
const float DELTA_TIME = 0.016f;

const float INTERACTION_RADIUS = 0.0f;
const float INTERACTION_STRENGTH = 0.0f;

bool upLastFrame = false;
bool downLastFrame = false;
bool oneLastFrame = false;
bool twoLastFrame = false;
bool vLastFrame = false;
bool bLastFrame = false;
bool nLastFrame = false;
bool mLastFrame = false;


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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Fluid Particles", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD\n";
		return -1;
	}

	glViewport(0, 0, WIDTH, HEIGHT);


	Shader shaderProgram("default.vert", "default.frag");

	Shader lineShader("line.vert", "line.frag");

	float line_boundary_x = BOUNDARY_X - PARTICLE_RADIUS;
	float line_boundary_y = BOUNDARY_Y - PARTICLE_RADIUS;

	std::vector<glm::vec3> boundaryLines = {
		{-line_boundary_x, -line_boundary_y, 0.0f}, { line_boundary_x, -line_boundary_y, 0.0f},
		{ line_boundary_x, -line_boundary_y, 0.0f}, { line_boundary_x,  line_boundary_y, 0.0f},
		{ line_boundary_x,  line_boundary_y, 0.0f}, {-line_boundary_x,  line_boundary_y, 0.0f},
		{-line_boundary_x,  line_boundary_y, 0.0f}, {-line_boundary_x, -line_boundary_y, 0.0f}
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

	Fluid fluid(PARTICLE_COUNT, PARTICLE_RADIUS, MASS, GRAVITY_ACCELERATION, COLLISION_DAMPING, SPACING, PRESSURE_MULTIPLIER, TARGET_DENSITY, SMOOTHING_RADIUS, SPATIAL_HASH_SIZE, INTERACTION_RADIUS, INTERACTION_STRENGTH, VISCOSITY_STRENGTH, NEAR_DENSITY_MULTIPLIER, BOUNDARY_X, BOUNDARY_Y, BOUNDARY_Z);

	std::vector<glm::vec3> circleVertices;
	std::vector<GLuint> circleIndices;
	CreateUnitCircle(circleVertices, circleIndices, CIRCLE_SEGMENTS, PARTICLE_RADIUS);

	VAO vao1;
	vao1.Bind();

	fluid.BindRenderBuffers();
	// Static circle mesh VBO
	VBO vboCircle(circleVertices.data(), circleVertices.size() * sizeof(glm::vec3));
	vao1.LinkVBO(vboCircle, 0); // layout(location = 0)

	EBO ebo1(circleIndices.data(), circleIndices.size() * sizeof(GLuint));

	ebo1.Bind();
	vao1.Unbind();
	vboCircle.Unbind();
	ebo1.Unbind();

	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// ------------------ MOUSE INTERACTION -----------------------
		fluid.SetIsInteracting(false);
		double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		// Convert to Normalized Device Coordinates [-1, 1]
		float x = 2.0f * static_cast<float>(mouseX) / WIDTH - 1.0f;
		float y = 1.0f - 2.0f * static_cast<float>(mouseY) / HEIGHT;
		glm::vec2 worldMousePos = glm::vec2(x, y);

		// Apply force on left-click
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			fluid.SetIsInteracting(true);
			fluid.SetInteractionPosition(glm::vec3(worldMousePos, 0.0f));
			fluid.SetInteractionStrength(INTERACTION_STRENGTH);
		}

		// Apply force on right-click
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
			fluid.SetIsInteracting(true);
			fluid.SetInteractionPosition(glm::vec3(worldMousePos, 0.0f));
			fluid.SetInteractionStrength(-INTERACTION_STRENGTH);
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
			fluid.SetTargetDensity(fluid.GetTargetDensity() + 10.0f);
		}
		oneLastFrame = (oneState == GLFW_PRESS);

		int twoState = glfwGetKey(window, GLFW_KEY_2);
		if (twoState == GLFW_PRESS && !twoLastFrame) {
			if (fluid.GetTargetDensity() > 3.0f)
				fluid.SetTargetDensity(fluid.GetTargetDensity() - 10.0f);
		}
		twoLastFrame = (twoState == GLFW_PRESS);

		int vState = glfwGetKey(window, GLFW_KEY_V);
		if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
			fluid.SetViscosityStrength(fluid.GetViscosityStrength() + 0.2f);
		}
		vLastFrame = (vState == GLFW_PRESS);

		int bState = glfwGetKey(window, GLFW_KEY_B);
		if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
			if (fluid.GetViscosityStrength() > 0.2f)
				fluid.SetViscosityStrength(fluid.GetViscosityStrength() - 0.2f);
		}
		bLastFrame = (bState == GLFW_PRESS);

		int nState = glfwGetKey(window, GLFW_KEY_N);
		if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
			fluid.SetNearDensityMultiplier(fluid.GetNearDensityMultiplier() + 0.1f);
		}
		nLastFrame = (nState == GLFW_PRESS);

		int mState = glfwGetKey(window, GLFW_KEY_M);
		if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
			if (fluid.GetNearDensityMultiplier() > 0.1f)
				fluid.SetNearDensityMultiplier(fluid.GetNearDensityMultiplier() - 0.1f);
		}
		mLastFrame = (mState == GLFW_PRESS);

		if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) 
				fluid.SetGravity(0.0f);

		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			fluid.SetGravity(GRAVITY_ACCELERATION);


		if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
			fluid.SetPaused(true);
		}

		if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
			fluid.SetPaused(false);
		}

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}
		// ------------------------------------------------------------

		fluid.Update(DELTA_TIME);

		shaderProgram.Activate();

		fluid.BindRenderBuffers();
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
	ebo1.Delete();
	shaderProgram.Delete();
	glDeleteVertexArrays(1, &boundaryVAO);
	glDeleteBuffers(1, &boundaryVBO);
	lineShader.Delete();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

