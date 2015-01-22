#ifndef LIGHT_H
#define LIGHT_H

#include "cpvs.h"
#include "Scene.h"

/**
 * Represents a directional Light.
 */
class DirectionalLight {
public:
	DirectionalLight(const vec3& direction, const AABB& bbox) : m_direction(direction) {
		calcViewTransform(bbox);
		calcProjection(bbox);
		m_viewProj = m_proj * m_view;
	}

	inline vec3 getDirection() const {
		return m_direction;
	}

	inline mat4 getViewTransform() const {
		return m_view;
	}

	inline mat4 getProjection() const {
		return m_proj;
	}

	inline mat4 getViewProj() const {
		return m_viewProj;
	}

	mat4 getSubProjection(const AABB& bbox, uint x, uint y, uint numSubDivisions) const;

private:
	void calcViewTransform(const AABB& bbox);
	
	void calcProjection(const AABB& bbox);

private:
	const vec3 m_direction;
	mat4 m_view;
	mat4 m_proj;
	mat4 m_viewProj;
};

#endif
