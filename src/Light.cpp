#include "Light.h"
#include <glm/ext.hpp>
#include <iostream>
using namespace std;

const vec3 up(0, 1, 0);

inline float getSceneSizeMax(const AABB& bbox) {
	float x = bbox.max.x - bbox.min.x;
	float y = bbox.max.y - bbox.min.y;
	float z = bbox.max.z - bbox.min.z;
	float tmp = std::max(x, y);
	return std::max(tmp, z);
}

mat4 DirectionalLight::calcLightView() const {
	vec3 position = direction * getSceneSizeMax(sceneBoundingBox) * 1.5f;
	return glm::lookAt(position, direction, up);
}

mat4 DirectionalLight::calcLightProj() const {
	constexpr float margin = 2.5f;

	float absX = std::abs(direction.x);
	float absY = std::abs(direction.y);
	float absZ = std::abs(direction.z);

	// 0, 1 or 2, i.e. x, y or z
	uint screenX, screenY;

	// Search for the "dominant dimension", screen x,y are choosen arbitrary from the remaining dimensions
	if (absX >= absY && absX >= absZ) {
		screenX = 1;
		screenY = 2;
	} else if (absY >= absX && absY >= absZ) {
		screenX = 0;
		screenY = 2;
	} else {
		screenX = 1;
		screenY = 0;
	}
		
	float minX = sceneBoundingBox.min[screenX] * margin;
	float minY = sceneBoundingBox.min[screenY] * margin;

	float maxX = sceneBoundingBox.max[screenX] * margin;
	float maxY = sceneBoundingBox.max[screenY] * margin;

	float zfar = getSceneSizeMax(sceneBoundingBox) * margin;

	return glm::ortho(minX, maxX, minY, maxY, 1.0f, zfar);
}

void DirectionalLight::updateLightViewProj() {
	lightViewProj = calcLightProj() * calcLightView();
}
