#ifndef LIGHT_H
#define LIGHT_H

#include "cpvs.h"

/**
 * Represents a directional Light.
 */
class DirectionalLight {
public:
	DirectionalLight() = default;
	DirectionalLight(vec3 col, vec3 dir) : color(col), direction(dir) { }
	~DirectionalLight() = default;

	vec3 color;
	vec3 direction;
};

#endif
