#ifndef LIGHT_H
#define LIGHT_H

#include "cpvs.h"

/**
 * Represents a directional Light.
 */
class DirectionalLight {
public:
	DirectionalLight(vec3 col, vec3 dir)
	   	: color(col), direction(dir), position(vec3(10, 50, 10)) { }
	~DirectionalLight() = default;

	mat4 getLightView() const;

	mat4 getLightProj() const;

public:
	vec3 color;
	vec3 direction;
	vec3 position;
};

#endif
