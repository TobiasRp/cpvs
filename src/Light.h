#ifndef LIGHT_H
#define LIGHT_H

#include "cpvs.h"

/**
 * Represents a directional Light.
 */
class DirectionalLight {
public:
	enum ShadowMode {
		NoShadows, ShadowMapping, CPVS
	};

public:
	DirectionalLight() = default;
	DirectionalLight(vec3 col, vec3 dir) : color(col), direction(dir), m_shadowMode(NoShadows) { }
	~DirectionalLight() = default;

	mat4 getLightView() const;

	mat4 getLightProj() const;

	inline void setShadowMode(ShadowMode mode) {
		m_shadowMode = mode;
	}

	bool shouldRenderShadowMap() const {
		return m_shadowMode != NoShadows;
	}

	bool useShadowMap() const {
		return m_shadowMode == ShadowMapping;
	}

	GLuint getShadowModeID() const {
		return m_shadowMode;
	}

	vec3 color;
	vec3 direction;

private:
	ShadowMode m_shadowMode;
};

#endif
