#include "Light.h"
#include <glm/ext.hpp>

const vec3 up(0, 1, 0);

mat4 DirectionalLight::getLightView() const {
	vec3 position = distance * direction;
	return glm::lookAt(position, direction, up);
}

mat4 DirectionalLight::getLightProj() const {
	return glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, znear, zfar);
}

mat4 DirectionalLight::getLightViewProj() const {
	return getLightProj() * getLightView();
}

