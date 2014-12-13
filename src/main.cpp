#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
using namespace std;

#include "cpvs.h"
#include "ShaderProgram.h"
#include "Camera.h"
#include "DeferredRenderer.h"
#include "PostProcess.h"

#include "AssimpScene.h"

/* Settings and globals */
const string defaultSceneFile = "../scenes/Desert_City/desert city.obj";

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

FreeCamera cam(45.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
unique_ptr<DeferredRenderer> renderSystem;
shared_ptr<PostProcess> fxaa;

bool renderShadowMap;

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
		break;
	case GLFW_KEY_F1:
			renderSystem->setPostProcess(fxaa);
			break;
	case GLFW_KEY_F2:
		/* Disables post process (if one exists) */
		renderSystem->removePostProcess();
		break;
	case GLFW_KEY_F3:
		/* Toggle rendering of scene / shadow map */
		if (action == GLFW_PRESS)
			renderShadowMap = !renderShadowMap;
		break;
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, GL_TRUE);
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
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		cerr << "Error: " << glewGetErrorString(err) << endl;
	}
}

/**
 * If the process was started with an argument, this is assumed to be the scene name,
 * or else the default scene will be loaded
 */
unique_ptr<AssimpScene> loadSceneFromArguments(int argc, char **argv) {
	string file;
	if (argc <= 1) {
		file = defaultSceneFile;
	} else {
		file = argv[1];
	}
	try {
		auto ptr = make_unique<AssimpScene>(file);
		return ptr;
	} catch (FileNotFound& exc) {
		cerr << "Specified scene file not found!\n";
		glfwTerminate();
		std::terminate();
	}
}

int main(int argc, char **argv) {
	auto window = initAndCreateWindow();
	initExtensions();

	vec3 lightDir = glm::normalize(vec3(1, -0.5, 0.1));
	DirectionalLight light(vec3(0.5, 0.5, 0.5), lightDir);

	renderSystem = make_unique<DeferredRenderer>(light, WINDOW_WIDTH, WINDOW_HEIGHT);

	fxaa = PostProcess::createFXAA(WINDOW_WIDTH, WINDOW_HEIGHT);
	renderSystem->setPostProcess(fxaa);

	cout << "Loading scene... ";
	auto scene = loadSceneFromArguments(argc, argv);
	cout << "finished." << endl;

	cam.setSpeed(4.0f);
	cam.setPosition(vec3(0, 3, -5));

	/* Render ShadowMap */
	auto sm = renderSystem->renderShadowMap(scene.get(), WINDOW_WIDTH, WINDOW_HEIGHT);

	while (!glfwWindowShouldClose(window)) {
		RenderProperties properties(cam.getView(), cam.getProjection());

		if (renderShadowMap)
			renderSystem->renderTexture(sm->getTexture());
		else
			renderSystem->render(properties, scene.get());
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
