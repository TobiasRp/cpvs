#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

#include <iostream>
using namespace std;
#include <chrono>
using namespace std::chrono;

#include <AntTweakBar.h>

#include "cpvs.h"
#include "ShaderProgram.h"
#include "Camera.h"
#include "DeferredRenderer.h"

#include "AssimpScene.h"

#include "MinMaxHierarchy.h"
#include "CompressedShadow.h"


/* Settings and globals */

const string defaultSceneFile = "../scenes/plane.obj";

const GLuint WINDOW_WIDTH = 512;
const GLuint WINDOW_HEIGHT = 512;

/* Shadow map and light settings */
const GLuint CPVS_SIZE      = 4096;//8192 * 1;
const GLuint REF_SM_SIZE    = 8192;
const vec3   lightDirection = {0.25, 1, 0};

/* Globals for camera and the deferred renderer */
FreeCamera cam(45.0f, WINDOW_WIDTH, WINDOW_HEIGHT, 1.0f, 100'000.0f);
unique_ptr<DeferredRenderer> renderSystem;

/* Settings controlled by AntTweakBar */
struct Settings {
	bool renderShadowMap;
	bool useReferenceShadows;

	float cameraSpeed;
};

Settings uiSettings;


void initCamera() {
	uiSettings.cameraSpeed = 3.0f;
	cam.setPosition(vec3(9, 5, 0));
}

void resize(GLFWwindow* window, int width, int height) {
	cam.setAspectRatio(width, height);
	glViewport(0, 0, width, height);
	renderSystem->resize(width, height);

	TwWindowSize(width, height);
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
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	// delegate to tweak bar
	auto twAction = (action == GLFW_PRESS) ? TW_MOUSE_PRESSED : TW_MOUSE_RELEASED;
	auto twButton = (button == GLFW_MOUSE_BUTTON_1) ? TW_MOUSE_LEFT : TW_MOUSE_RIGHT;
	TwMouseButton(twAction, twButton);
}

void setMousePosCallback(GLFWwindow* window, double xpos, double ypos) {
	TwMouseMotion(xpos, ypos);
}

GLFWwindow* initAndCreateWindow() {
	if (!glfwInit())
		cerr << "Error: Could not initialize GLFW" << endl;

	auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "CPVS", nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		cerr << "Error: GLFW could not create window" << endl;
	}

	glfwSetWindowSizeCallback(window, resize);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, setMousePosCallback);
	glfwMakeContextCurrent(window);
	return window;
}

void initExtensions() {
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		cerr << "Error: " << glewGetErrorString(err) << endl;
	}
}

void initUiSettings() {
	uiSettings.renderShadowMap = false;
	uiSettings.useReferenceShadows = false;
}

void initTweakBar() {
	TwInit(TW_OPENGL_CORE, NULL);

	TwWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	auto twBar = TwNewBar("CPVS Settings");
	TwAddVarRW(twBar, "Render shadow map", TW_TYPE_BOOLCPP, &uiSettings.renderShadowMap, nullptr);
	TwAddVarRW(twBar, "Reference shadow mapping", TW_TYPE_BOOLCPP, &uiSettings.useReferenceShadows, nullptr);

	TwAddSeparator(twBar, nullptr, nullptr);
	TwAddVarRW(twBar, "Camera speed", TW_TYPE_FLOAT, &uiSettings.cameraSpeed, " min=0.1 step=1");
}

inline void printDurationToNow(high_resolution_clock::time_point start) {
	auto t1 = high_resolution_clock::now();
	cout << duration_cast<milliseconds>(t1 - start).count() << "msec\n";
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
		cout << "Loading scene... "; cout.flush();
		auto t0 = high_resolution_clock::now();
		auto ptr = make_unique<AssimpScene>(file);
		cout << "done after ";
		printDurationToNow(t0);
		return ptr;
	} catch (FileNotFound& exc) {
		cerr << "Specified scene file not found!\n";
		glfwTerminate();
		std::exit(EXIT_FAILURE);
	}
}

void initRenderSystem(const Scene* scene) {
	vec3 direction = glm::normalize(lightDirection);
	DirectionalLight light(direction, scene->getBoundingBox());
	renderSystem = make_unique<DeferredRenderer>(light, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void createPrecomputedShadows(const Scene* scene) {
	cout << "Precomputing shadows... "; cout.flush();
	auto t0 = chrono::high_resolution_clock::now();
	renderSystem->precomputeShadows(scene, CPVS_SIZE);
	cout << " done after ";
	printDurationToNow(t0);
}

int main(int argc, char **argv) {
	auto window = initAndCreateWindow();
	initExtensions();

	initUiSettings();
	initTweakBar();

	auto scene = loadSceneFromArguments(argc, argv);

	initCamera();
	initRenderSystem(scene.get());

	createPrecomputedShadows(scene.get());

	// Render reference shadow map 
	auto refSM  = renderSystem->renderShadowMap(scene.get(), REF_SM_SIZE);
	auto refTex = refSM->getTexture();
	renderSystem->setShadow(refTex);

	while (!glfwWindowShouldClose(window)) {
		RenderProperties properties(cam.getView(), cam.getProjection());

		renderSystem->useReferenceShadows(uiSettings.useReferenceShadows);

		if (uiSettings.renderShadowMap)
			renderSystem->renderDepthTexture(refTex.get());
		else
			renderSystem->render(properties, scene.get());

		cam.setSpeed(uiSettings.cameraSpeed);
		TwDraw();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	TwDeleteAllBars();
	TwTerminate();
	glfwTerminate();
	return 0;
}
