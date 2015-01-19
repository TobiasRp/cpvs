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

mat4 DirectionalLight::getSubViewTransform(const AABB& bbox, const vec3 subPos, uint numSubDivisions) {
	vec3 subSize = (bbox.max - bbox.min) / numSubDivisions;
	vec3 subCenter = bbox.min + (subSize * 0.5f);

	vec3 position = subCenter + subSize * subPos;
	vec3 target = bbox.getCenter() + m_direction;

	return glm::lookAt(position, target, up);
}

mat4 DirectionalLight::getSubProjection(const AABB& bbox, const mat4& viewTransform, const vec3 subPos,
		uint numSubDivisions) {
	vec4 minLS = viewTransform * vec4(bbox.min * margin, 1.0);
	vec4 maxLS = viewTransform * vec4(bbox.max * margin, 1.0);

	vec3 subSize = (vec3(maxLS) - vec3(minLS)) / numSubDivisions;
	vec3 subMin = vec3(minLS) + subPos * subSize;

	return glm::ortho(subMin.x, subMin.x + subSize.x, subMin.y, subMin.y + subSize.y, subMin.z, subMin.z + subSize.z);
}
