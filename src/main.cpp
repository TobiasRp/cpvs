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
GLuint cpvs_size = 4096;
GLuint pcf_size = 1;

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

inline void closeApp(int returnCode) {
	glfwTerminate();
	std::exit(EXIT_FAILURE);
}

/**
 * If the process was started with an argument, this is assumed to be the scene name,
 * or else the default scene will be loaded
 */
inline unique_ptr<Scene> loadSceneFromArguments(const string& file) {
	try {
		cout << "Loading scene... "; cout.flush();
		auto t0 = high_resolution_clock::now();
		auto ptr = AssimpScene::loadScene(file);
		cout << "done after ";
		printDurationToNow(t0);
		return ptr;
	} catch (FileNotFound& exc) {
		cerr << "Specified scene file not found!\n";
		closeApp(EXIT_FAILURE);
		return nullptr; // avoid compiler warning
	}
}

inline void printHelpAndExit() {
	cout << "CPVS Usage:\n"
		 << "\t--help Prints this help test and exits\n"
		 << "\t--size=[size of precomputed shadow, e.g. 8196. Must be a power of two]\n"
		 << "\t--pcf=[size of PCF kernel]\n"
		 << "\tpath to scene file or default file which will be loaded"
	   	 << endl;
	closeApp(EXIT_SUCCESS);
}

inline uint parseSize(const string& sizeStr, bool testPowerOfTwo) {
	uint res;
	try {
		res = std::stoul(sizeStr);
		if (testPowerOfTwo && (!isPowerOfTwo(res) || res < 8))
			throw std::invalid_argument("Must be power of two and at least 8");
	} catch (std::exception& exc) {
		cerr << "Invalid size specified (" << exc.what() << ")\n";
		closeApp(EXIT_FAILURE);
	}
	return res;
}

unique_ptr<Scene> parseArguments(int argc, char **argv) {
	string sceneFile = defaultSceneFile;

	for (int paramNr = 1; paramNr < argc; ++paramNr) {
		string param(argv[paramNr]);

		if (param == "--help") {
			printHelpAndExit();
		} else if (param.substr(0, 6) == "--size") {
			cpvs_size = parseSize(&argv[paramNr][7], true);
		} else if (param.substr(0, 5) == "--pcf") {
			pcf_size = parseSize(&argv[paramNr][6], false);
		}else {
			sceneFile = param;
		}
	}

	return loadSceneFromArguments(sceneFile);
}

void initRenderSystem(const Scene* scene) {
	vec3 direction = glm::normalize(lightDirection);
	DirectionalLight light(direction, scene->boundingBox);
	renderSystem = make_unique<DeferredRenderer>(light, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void createPrecomputedShadows(const Scene* scene) {
	cout << "Precomputing shadows... "; cout.flush();
	auto t0 = chrono::high_resolution_clock::now();
	renderSystem->precomputeShadows(scene, cpvs_size, pcf_size);
	cout << "\n... done after ";
	printDurationToNow(t0);
}

int main(int argc, char **argv) {
	auto window = initAndCreateWindow();
	initExtensions();

	initUiSettings();
	initTweakBar();

	initCamera();

	auto scene = parseArguments(argc, argv);

	initRenderSystem(scene.get());

	createPrecomputedShadows(scene.get());

	// Render reference shadow map 
	auto refSM  = renderSystem->renderShadowMap(scene.get(), REF_SM_SIZE);
	auto refTex = refSM->getTexture();
	renderSystem->setShadow(refTex);

	while (!glfwWindowShouldClose(window)) {
		renderSystem->useReferenceShadows(uiSettings.useReferenceShadows);

		if (uiSettings.renderShadowMap)
			renderSystem->renderDepthTexture(refTex.get());
		else
			renderSystem->render(&cam, scene.get());

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
