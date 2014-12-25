#ifndef LIGHT_H
#define LIGHT_H

#include "cpvs.h"

/**
 * Represents a directional Light.
 */
class DirectionalLight {
public:
	DirectionalLight(vec3 col, vec3 dir, float dist = 900)
	   	: color(col), direction(dir), distance(dist), znear(1), zfar(1000) { }
	~DirectionalLight() = default;

	mat4 getLightView() const;

	mat4 getLightProj() const;

public:
	vec3 color;
	vec3 direction;
	
	// Rendering of the shadow map needs a position, this is (0, 0, 0) + distance * direction
	float distance; 

	// For the shadow map rendering
	float znear, zfar;
};

#endif
