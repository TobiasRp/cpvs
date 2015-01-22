#include "Light.h"
#include <glm/ext.hpp>
#include <iostream>
using namespace std;

const vec3 up(0, 1, 0);

constexpr float margin = 1.2f;

void DirectionalLight::calcViewTransform(const AABB& bbox) {
	vec3 position = bbox.getCenter();
	vec3 target = position + m_direction;
	m_view = glm::lookAt(position, target, up);
}

void DirectionalLight::calcProjection(const AABB& bbox) {
	vec4 minLS = m_view * vec4(bbox.min * margin, 1.0);
	vec4 maxLS = m_view * vec4(bbox.max * margin, 1.0);

	m_proj = glm::ortho(minLS.x, maxLS.x, minLS.y, maxLS.y, minLS.z, maxLS.z);
}

mat4 DirectionalLight::getSubProjection(const AABB& bbox, uint x, uint y, uint numSubDivisions) const {
	vec4 minLS = m_view * vec4(bbox.min * margin, 1.0);
	vec4 maxLS = m_view * vec4(bbox.max * margin, 1.0);

	vec3 subSize = (vec3(maxLS) - vec3(minLS)) / numSubDivisions;
	float subMinX = minLS.x + x * subSize.x;
	float subMinY = minLS.y + y * subSize.y;

	return glm::ortho(subMinX, subMinX + subSize.x, subMinY, subMinY + subSize.y, minLS.z, maxLS.z);
}
