#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
using namespace std;

#include "cpvs.h"
#include "ShaderProgram.h"
#include "Camera.h"

#include "AssimpRenderer.h"

FreeCamera cam(45.0f, WINDOW_WIDTH, WINDOW_HEIGHT);

void resize(GLFWwindow* window, int width, int height) {
	cam.setAspectRatio(width, height);
	glViewport(0, 0, width, height);
}

GLFWwindow* initAndCreateWindow() {
	if (!glfwInit())
		cerr << "Error: Could not initialize GLFW" << endl;

	auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "glCompute", nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		cerr << "Error: GLFW could not create window" << endl;
	}

	glfwSetWindowSizeCallback(window, resize);

	glfwMakeContextCurrent(window);
	return window;
}

void initExtensions() {
	cout << "Initializing OpenGL extensions..." << endl;
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		cerr << "Error: " << glewGetErrorString(err) << endl;
	}
}

unique_ptr<ShaderProgram> loadForwardShader() {
	unique_ptr<ShaderProgram> forward = make_unique<ShaderProgram>();
	try {
		forward->addShaderFromFile(GL_VERTEX_SHADER, "../shader/forward.vert");
		forward->addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/forward.frag");
	} catch (ShaderException &exc) {
		cout << exc.what() << endl;
		glfwTerminate();
		std::terminate();
	}
	forward->link();
	return forward;
}

int main(int argc, char **argv) {
	auto window = initAndCreateWindow();
	initExtensions();

	auto forward = loadForwardShader();
	forward->bind();

	AssimpRenderer sponza("../../Sponza/sponza.obj");

	cam.setPosition(vec3(0, 0, -5));

	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderProperties properties(forward.get(), cam.getView(), cam.getProjection());
		sponza.render(properties);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
