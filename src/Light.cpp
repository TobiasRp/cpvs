#include "Light.h"
#include <glm/ext.hpp>

const vec3 lightPos(0, 50, 0);
const vec3 up(0, 1, 0);

mat4 DirectionalLight::getLightView() const {
	return glm::lookAt(lightPos, direction, up);
}

mat4 DirectionalLight::getLightProj() const {
	return glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1000.0f);
}

