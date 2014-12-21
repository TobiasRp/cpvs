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

#include "MinMaxHierarchy.h"
#include "CompressedShadow.h"
#include <chrono>
using namespace std::chrono;

/* Settings and globals */
const string defaultSceneFile = "../scenes/sibenik/sibenik.obj";

const GLuint WINDOW_WIDTH = 512;
const GLuint WINDOW_HEIGHT = 512;

const GLuint SM_SIZE = 8192;

FreeCamera cam(45.0f, WINDOW_WIDTH, WINDOW_HEIGHT, 0.2, 100'000.0f);
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

void printDurationToNow(high_resolution_clock::time_point start) {
	auto t1 = high_resolution_clock::now();
	cout << duration_cast<milliseconds>(t1 - start).count() << "msec\n";
}

int main(int argc, char **argv) {
	auto window = initAndCreateWindow();
	initExtensions();

	vec3 lightDir = glm::normalize( - vec3(1, 0.5, 0.1));
	DirectionalLight light(vec3(0.5, 0.5, 0.5), lightDir);

	renderSystem = make_unique<DeferredRenderer>(light, WINDOW_WIDTH, WINDOW_HEIGHT);

	fxaa = PostProcess::createFXAA(WINDOW_WIDTH, WINDOW_HEIGHT);
	renderSystem->setPostProcess(fxaa);

	cout << "Loading scene... "; cout.flush();
	auto t0 = high_resolution_clock::now();
	auto scene = loadSceneFromArguments(argc, argv);
	cout << "done after ";
	printDurationToNow(t0);

	cam.setSpeed(3.0f);
	cam.setPosition(vec3(10, 10, 10));
	cam.rotate(180, -10, 0);

	/* Render shadow map and create min-max hierarchy*/
	auto sm = renderSystem->renderShadowMap(scene.get(), SM_SIZE);
	auto smImg = sm->createImageF();
	cout << "Loading min-max hierarchy..."; cout.flush();
	t0 = high_resolution_clock::now();
	MinMaxHierarchy mm(smImg);
	cout << " done after ";
	printDurationToNow(t0);

	cout << "Compressing shadow... "; cout.flush();
	t0 = chrono::high_resolution_clock::now();
	auto shadow = CompressedShadow::create(mm);
	cout << " done after ";
	printDurationToNow(t0);


	auto level1 = mm.getLevel(5);
	auto texLevel1 = std::make_shared<Texture2D>(*level1);

	while (!glfwWindowShouldClose(window)) {
		RenderProperties properties(cam.getView(), cam.getProjection());

		if (renderShadowMap)
			renderSystem->renderTexture(texLevel1);
		else
			renderSystem->render(properties, scene.get());
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
