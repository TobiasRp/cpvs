#include "Light.h"
#include <glm/ext.hpp>

const vec3 up(0, 1, 0);

mat4 DirectionalLight::calcLightView() const {
	vec3 position = distance * direction;
	return glm::lookAt(position, direction, up);
}

mat4 DirectionalLight::calcLightProj() const {
	return glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, znear, zfar);
	//return glm::perspective(45.0f, 1.0f, znear, zfar);
}

void DirectionalLight::updateLightViewProj() {
	lightViewProj = calcLightProj() * calcLightView();
}
