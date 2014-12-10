#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <memory>
#include <iostream>
using namespace std;

#include "cpvs.h"
#include "ShaderProgram.h"

GLFWwindow* initAndCreateWindow() {
	if (!glfwInit())
		cerr << "Error: Could not initialize GLFW" << endl;

	auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "glCompute", nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		cerr << "Error: GLFW could not create window" << endl;
	}
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

int main(int argc, char **argv) {
	auto window = initAndCreateWindow();
	initExtensions();

	while (!glfwWindowShouldClose(window)) {

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
