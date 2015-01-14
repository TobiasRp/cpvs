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

/**
 * Decides which dimensions are the screen coordinates x, y for a viewer aligned with the given direction.
 */
std::pair<uint, uint> getScreenXY(const vec3& direction) {
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

	return make_pair(screenX, screenY);
}

void DirectionalLight::getMinMaxValues(float* minX, float* minY, float* maxX, float* maxY, float* zfar) const {
	constexpr float margin = 2.5f;

	auto screen = getScreenXY(direction);
	uint screenX = get<0>(screen);
	uint screenY = get<1>(screen);
		
	*minX = sceneBoundingBox.min[screenX] * margin;
	*minY = sceneBoundingBox.min[screenY] * margin;

	*maxX = sceneBoundingBox.max[screenX] * margin;
	*maxY = sceneBoundingBox.max[screenY] * margin;

	*zfar = getSceneSizeMax(sceneBoundingBox) * margin;
}

mat4 DirectionalLight::calcLightProj() const {
	float minX, minY, maxX, maxY, zfar;
	getMinMaxValues(&minX, &minY, &maxX, &maxY, &zfar);

	return glm::ortho(minX, maxX, minY, maxY, 1.0f, zfar);
}

mat4 DirectionalLight::calcLightProj(uint x, uint y, uint z, uint resolution) const {
	float minX, minY, maxX, maxY, zfar;
	getMinMaxValues(&minX, &minY, &maxX, &maxY, &zfar);

	float sizeX = std::abs(maxX - minX) / resolution;
	float sizeY = std::abs(maxY - minY) / resolution;
	float sizeZ = std::abs(zfar - 1.0f) / resolution;

	float subMinX = minX + sizeX * x;
	float subMaxX = subMinX + sizeX;

	float subMinY = minY + sizeY * y;
	float subMaxY = subMinY + sizeY;;

	return glm::ortho(subMinX, subMaxX, subMinY, subMaxY, 1.0f + sizeZ * z, 1.0f + sizeZ * (z + 1));
}

void DirectionalLight::updateLightViewProj() {
	lightViewProj = calcLightProj() * calcLightView();
}
