#include "Light.h"
#include <glm/ext.hpp>

const vec3 up(0, 1, 0);

const float znear = 1.0f;
const float zfar = 1000.0f;

mat4 DirectionalLight::getLightView() const {
	return glm::lookAt(position, direction, up);
}

mat4 DirectionalLight::getLightProj() const {
	return glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, znear, zfar);
}

