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
#include "tiny_obj_loader.h"
#include "Camera.h"

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

const unsigned int WIDTH = 1500, HEIGHT = 900;
const unsigned int PARTICLE_COUNT = 1024 * 32;
const unsigned int SPATIAL_HASH_SIZE = PARTICLE_COUNT * 8;
const float PARTICLE_RADIUS = 0.006f;
const float MASS = 0.06f;
const float GRAVITY_ACCELERATION = 1.2f;
const float COLLISION_DAMPING = 0.3f;
const float BOUNDARY_X = 1.2f;
const float BOUNDARY_Y = 0.6f;
const float BOUNDARY_Z = 0.6f;
const float SPACING = 0.02f;
const float SMOOTHING_RADIUS = 0.08f;
const float PRESSURE_MULTIPLIER = 2.0f;
const float TARGET_DENSITY = 1200.0f;
const float VISCOSITY_STRENGTH = 0.3f;
const float NEAR_DENSITY_MULTIPLIER = 0.2f;
const float DELTA_TIME = 0.016f;

const float INTERACTION_RADIUS = 0.25f;
const float INTERACTION_STRENGTH = 7.0f;

bool upLastFrame = false;
bool downLastFrame = false;
bool oneLastFrame = false;
bool twoLastFrame = false;
bool vLastFrame = false;
bool bLastFrame = false;
bool nLastFrame = false;
bool mLastFrame = false;

const float FOV = 45.0f;
glm::vec3 cameraPosition(0.0f, 1.0f, 2.5f);
glm::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

Camera camera(cameraPosition, cameraTarget, cameraUp);

static void CreateUVSphere(std::vector<glm::vec3>& verts,
	std::vector<GLuint>& inds,
	int latSegs = 16,
	int longSegs = 16,
	float radius = 0.1f)
{
	verts.clear();
	inds.clear();

	// make vertices
	for (int y = 0; y <= latSegs; ++y) {
		float v = float(y) / latSegs;             
		float theta = v * glm::pi<float>();       
		float sinT = std::sin(theta), cosT = std::cos(theta);

		for (int x = 0; x <= longSegs; ++x) {
			float u = float(x) / longSegs;       
			float phi = u * glm::two_pi<float>();    
			float sinP = std::sin(phi), cosP = std::cos(phi);

			glm::vec3 p;
			p.x = radius * sinT * cosP;
			p.y = radius * cosT;
			p.z = radius * sinT * sinP;
			verts.push_back(p);
		}
	}

	for (int y = 0; y < latSegs; ++y) {
		for (int x = 0; x < longSegs; ++x) {
			int i0 = y * (longSegs + 1) + x;
			int i1 = (y + 1) * (longSegs + 1) + x;
			int i2 = y * (longSegs + 1) + (x + 1);
			int i3 = (y + 1) * (longSegs + 1) + (x + 1);

			
			inds.push_back(i0);
			inds.push_back(i1);
			inds.push_back(i2);
			
			inds.push_back(i2);
			inds.push_back(i1);
			inds.push_back(i3);
		}
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

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD\n";
		return -1;
	}

	glViewport(0, 0, WIDTH, HEIGHT);
	glEnable(GL_DEPTH_TEST);

	Shader shaderProgram("default.vert", "default.frag");

	Shader lineShader("line.vert", "line.frag");

	float line_boundary_x = BOUNDARY_X - PARTICLE_RADIUS;
	float line_boundary_y = BOUNDARY_Y - PARTICLE_RADIUS;
	float line_boundary_z = BOUNDARY_Z - PARTICLE_RADIUS;

	std::vector<glm::vec3> boundaryLines = {
		// bottom rectangle
		{-line_boundary_x, -line_boundary_y, -line_boundary_z}, { line_boundary_x, -line_boundary_y, -line_boundary_z},
		{ line_boundary_x, -line_boundary_y, -line_boundary_z}, { line_boundary_x,  line_boundary_y, -line_boundary_z},
		{ line_boundary_x,  line_boundary_y, -line_boundary_z}, {-line_boundary_x,  line_boundary_y, -line_boundary_z},
		{-line_boundary_x,  line_boundary_y, -line_boundary_z}, {-line_boundary_x, -line_boundary_y, -line_boundary_z},

		// top rectangle
		{-line_boundary_x, -line_boundary_y,  line_boundary_z}, { line_boundary_x, -line_boundary_y,  line_boundary_z},
		{ line_boundary_x, -line_boundary_y,  line_boundary_z}, { line_boundary_x,  line_boundary_y,  line_boundary_z},
		{ line_boundary_x,  line_boundary_y,  line_boundary_z}, {-line_boundary_x,  line_boundary_y,  line_boundary_z},
		{-line_boundary_x,  line_boundary_y,  line_boundary_z}, {-line_boundary_x, -line_boundary_y,  line_boundary_z},

		// vertical edges
		{-line_boundary_x, -line_boundary_y, -line_boundary_z}, {-line_boundary_x, -line_boundary_y,  line_boundary_z},
		{ line_boundary_x, -line_boundary_y, -line_boundary_z}, { line_boundary_x, -line_boundary_y,  line_boundary_z},
		{ line_boundary_x,  line_boundary_y, -line_boundary_z}, { line_boundary_x,  line_boundary_y,  line_boundary_z},
		{-line_boundary_x,  line_boundary_y, -line_boundary_z}, {-line_boundary_x,  line_boundary_y,  line_boundary_z},
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

	std::vector<glm::vec3> sphereVertices;
	std::vector<GLuint> sphereIndices;
	CreateUVSphere(sphereVertices, sphereIndices, 4, 4, 1.0f); // I am not sure about using 1.0f scale or PARTICLE_RADIUS

	VAO vao1;
	vao1.Bind();

	fluid.BindRenderBuffers();
	// Static circle mesh VBO
	VBO vboSphere(sphereVertices.data(), sphereVertices.size() * sizeof(glm::vec3));
	vao1.LinkVBO(vboSphere, 0); // layout(location = 0)

	EBO eboSphere(sphereIndices.data(), sphereIndices.size() * sizeof(GLuint));

	eboSphere.Bind();
	vao1.Unbind();
	vboSphere.Unbind();
	eboSphere.Unbind();

	double lastTime = glfwGetTime();
	int  nbFrames = 0;

	while (!glfwWindowShouldClose(window)) {
		double currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 1.0) {
			int fps = nbFrames;
			std::string title = "Fluid Particles -> FPS: " + std::to_string(fps);
			glfwSetWindowTitle(window, title.c_str());
			nbFrames = 0;
			lastTime += 1.0;
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// ------------------ MOUSE INTERACTION -----------------------
		fluid.SetIsInteracting(false);
		/*double mouseX, mouseY;
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
			fluid.SetInteractionRadius(INTERACTION_RADIUS);
		}

		// Apply force on right-click
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
			fluid.SetIsInteracting(true);
			fluid.SetInteractionPosition(glm::vec3(worldMousePos, 0.0f));
			fluid.SetInteractionStrength(-INTERACTION_STRENGTH);
			fluid.SetInteractionRadius(INTERACTION_RADIUS);
		}
		// ------------------------------------------------------------*/


		// ------------------ KEYBOARD CONTROLS -----------------------
		int upState = glfwGetKey(window, GLFW_KEY_UP);
		if (upState == GLFW_PRESS && !upLastFrame) {
			fluid.SetPressureMultiplier(fluid.GetPressureMultiplier() + 0.5f);
		}
		upLastFrame = (upState == GLFW_PRESS);

		int downState = glfwGetKey(window, GLFW_KEY_DOWN);
		if (downState == GLFW_PRESS && !downLastFrame) {
			fluid.SetPressureMultiplier(fluid.GetPressureMultiplier() - 0.5f);
		}
		downLastFrame = (downState == GLFW_PRESS);

		int oneState = glfwGetKey(window, GLFW_KEY_1);
		if (oneState == GLFW_PRESS && !oneLastFrame) {
			fluid.SetTargetDensity(fluid.GetTargetDensity() + 50.0f);
		}
		oneLastFrame = (oneState == GLFW_PRESS);

		int twoState = glfwGetKey(window, GLFW_KEY_2);
		if (twoState == GLFW_PRESS && !twoLastFrame) {
			if (fluid.GetTargetDensity() > 3.0f)
				fluid.SetTargetDensity(fluid.GetTargetDensity() - 50.0f);
		}
		twoLastFrame = (twoState == GLFW_PRESS);

		int vState = glfwGetKey(window, GLFW_KEY_V);
		if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
			fluid.SetViscosityStrength(fluid.GetViscosityStrength() + 0.1f);
		}
		vLastFrame = (vState == GLFW_PRESS);

		int bState = glfwGetKey(window, GLFW_KEY_B);
		if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
			if (fluid.GetViscosityStrength() > 0.2f)
				fluid.SetViscosityStrength(fluid.GetViscosityStrength() - 0.1f);
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

		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(FOV), float(WIDTH) / HEIGHT, 0.01f, 100.0f);
		glm::mat4 model = glm::mat4(1.0f);

		shaderProgram.Activate();
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));


		fluid.BindRenderBuffers();
		vao1.Bind();
		glUniform1f(glGetUniformLocation(shaderProgram.ID, "scale"), PARTICLE_RADIUS);
		glDrawElementsInstanced(GL_TRIANGLES, GLsizei(sphereIndices.size()), GL_UNSIGNED_INT, 0, PARTICLE_COUNT);

		lineShader.Activate();
		glUniformMatrix4fv(glGetUniformLocation(lineShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(lineShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(lineShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

		glBindVertexArray(boundaryVAO);
		glDrawArrays(GL_LINES, 0, boundaryLines.size());
		glBindVertexArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	vao1.Delete();
	vboSphere.Delete();
	eboSphere.Delete();
	shaderProgram.Delete();
	glDeleteVertexArrays(1, &boundaryVAO);
	glDeleteBuffers(1, &boundaryVBO);
	lineShader.Delete();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}