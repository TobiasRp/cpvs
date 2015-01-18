#include "Light.h"
#include <glm/ext.hpp>
#include <iostream>
using namespace std;

const vec3 up(0, 1, 0);

constexpr float margin = 1.5f;

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

mat4 DirectionalLight::getSubProjection(const AABB& bbox, uint x, uint y, uint z, uint resolution) {
	vec4 minLS = m_view * vec4(bbox.min * margin, 1.0);
	vec4 maxLS = m_view * vec4(bbox.max * margin, 1.0);

	vec3 subSize = (vec3(maxLS) - vec3(minLS)) / resolution;

	vec3 subMin = vec3(minLS.x + subSize.x * x, minLS.y + subSize.y * y, minLS.z + subSize.z * z);

	return glm::ortho(subMin.x, subMin.x + subSize.x, subMin.y, subMin.y + subSize.y, subMin.z,
			subMin.z + subSize.z);
}
