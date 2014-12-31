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
#include "PostProcess.h"

#include "AssimpScene.h"

#include "MinMaxHierarchy.h"
#include "CompressedShadow.h"

/* Settings and globals */
const string defaultSceneFile = "../scenes/plane.obj";

const GLuint WINDOW_WIDTH = 512;
const GLuint WINDOW_HEIGHT = 512;


/* Shadow map and light settings */
const GLuint SM_SIZE        = 8192;
const float  lightDistance  = 680;
const vec3   lightDirection = {0.25, 1, 0};

FreeCamera cam(45.0f, WINDOW_WIDTH, WINDOW_HEIGHT, 1, 1000.0f);
unique_ptr<DeferredRenderer> renderSystem;
shared_ptr<PostProcess> fxaa;

struct Settings {
	bool renderShadowMap;
	uint smLevel;

	bool useReferenceShadows;
};

Settings uiSettings;

void initCamera() {
	cam.setSpeed(3.0f);
	cam.setPosition(vec3(-4, 5, 0));
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
	case GLFW_KEY_F1:
			renderSystem->setPostProcess(fxaa);
			break;
	case GLFW_KEY_F2:
		/* Disables post process (if one exists) */
		renderSystem->removePostProcess();
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

	string minMaxSettings("min=0 max=");
	minMaxSettings.append(to_string((int)log2(SM_SIZE)));
	TwAddVarRW(twBar, "Shadow map level", TW_TYPE_UINT32, &uiSettings.smLevel, minMaxSettings.c_str());

	TwAddSeparator(twBar, "firstSep", nullptr);

	TwAddVarRW(twBar, "Reference shadow mapping", TW_TYPE_BOOLCPP, &uiSettings.useReferenceShadows, nullptr);
}

void printDurationToNow(high_resolution_clock::time_point start) {
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
		std::terminate();
	}
}

void initRenderSystem() {
	vec3 direction = glm::normalize(lightDirection);
	DirectionalLight light(vec3(0.65, 0.65, 0.65), direction, lightDistance);

	renderSystem = make_unique<DeferredRenderer>(light, WINDOW_WIDTH, WINDOW_HEIGHT);

	fxaa = PostProcess::createFXAA(WINDOW_WIDTH, WINDOW_HEIGHT);
}

MinMaxHierarchy createPrecomputedShadows(const Scene* scene) {
	auto sm = renderSystem->renderShadowMap(scene, SM_SIZE);
	cout << "Loading min-max hierarchy..."; cout.flush();
	auto t0 = high_resolution_clock::now();
	MinMaxHierarchy mm(sm->createImageF());
	cout << " done after ";
	printDurationToNow(t0);

	cout << "Compressing shadow... "; cout.flush();
	t0 = chrono::high_resolution_clock::now();
	auto shadow = CompressedShadow::create(mm);
	cout << " done after ";
	printDurationToNow(t0);

	shadow->freeDagOnCPU();
	renderSystem->setShadow(std::move(shadow));

	return mm;
}

int main(int argc, char **argv) {
	auto window = initAndCreateWindow();
	initExtensions();

	initUiSettings();
	initTweakBar();

	auto scene = loadSceneFromArguments(argc, argv);

	initCamera();
	initRenderSystem();

	auto mm = createPrecomputedShadows(scene.get());

	auto level     = mm.getLevel(0);
	auto texLevel  = std::make_shared<Texture2D>(*level);
	uint lastLevel = 0;

	renderSystem->setShadow(texLevel);

	while (!glfwWindowShouldClose(window)) {
		RenderProperties properties(cam.getView(), cam.getProjection());

		if (uiSettings.smLevel != lastLevel) {
			// update sm texture only when necessary
			level = mm.getLevel(uiSettings.smLevel);
			texLevel = std::make_shared<Texture2D>(*level);
			lastLevel = uiSettings.smLevel;
		}

		renderSystem->useReferenceShadows(uiSettings.useReferenceShadows);

		if (uiSettings.renderShadowMap)
			renderSystem->renderDepthTexture(texLevel.get());
		else
			renderSystem->render(properties, scene.get());
		
		TwDraw();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	TwDeleteAllBars();
	TwTerminate();
	glfwTerminate();
	return 0;
}
