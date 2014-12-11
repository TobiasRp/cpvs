#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
using namespace std;

#include "cpvs.h"
#include "ShaderProgram.h"
#include "Camera.h"
#include "DeferredRenderer.h"

#include "AssimpScene.h"

FreeCamera cam(45.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
unique_ptr<DeferredRenderer> renderSystem;

void resize(GLFWwindow* window, int width, int height) {
	cam.setAspectRatio(width, height);
	glViewport(0, 0, width, height);
	renderSystem->resize(width, height);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	switch(key) {
	case GLFW_KEY_W:
		cam.walk(1);
		break;
	case GLFW_KEY_S:
		cam.walk(-1);
		break;
	case GLFW_KEY_A:
		cam.strafe(-1);
		break;
	case GLFW_KEY_D:
		cam.strafe(1);
		break;
	case GLFW_KEY_Q:
		cam.rotate(5.0f, 0, 0);
		break;
	case GLFW_KEY_E:
		cam.rotate(-5.0f, 0, 0);
		break;
	case GLFW_KEY_Z:
		cam.lift(1);
		break;
	case GLFW_KEY_X:
		cam.lift(-1);
	}
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
	glfwSetKeyCallback(window, keyCallback);
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

unique_ptr<AssimpScene> loadSceneFromArguments(int argc, char **argv) {
	string file;
	if (argc <= 1) {
		file = "../scenes/Desert_City/desert city.obj";
	} else {
		file = argv[1];
	}
	try {
		auto ptr = make_unique<AssimpScene>(file);
		return ptr;
	} catch (FileNotFound& exc) {
		cout << "Specified scene file not found!\n";
		glfwTerminate();
		std::terminate();
	}
}

int main(int argc, char **argv) {
	auto window = initAndCreateWindow();
	initExtensions();

	auto forward = loadForwardShader();
	forward->bind();

	auto scene = loadSceneFromArguments(argc, argv);

	renderSystem = make_unique<DeferredRenderer>(WINDOW_WIDTH, WINDOW_HEIGHT);

	cam.setSpeed(4.0f);
	cam.setPosition(vec3(0, 0, -5));

	glEnable(GL_DEPTH_TEST);
//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	while (!glfwWindowShouldClose(window)) {
		RenderProperties properties(forward.get(), cam.getView(), cam.getProjection());
		renderSystem->render(properties, scene.get());
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
