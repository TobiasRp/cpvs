#ifndef LIGHT_H
#define LIGHT_H

#include "cpvs.h"
#include "Scene.h"

/**
 * Represents a directional Light.
 */
class DirectionalLight {
public:
	DirectionalLight(vec3 col, vec3 dir)
	   	: color(col), direction(dir) { }
	~DirectionalLight() = default;

	mat4 calcLightView() const;

	mat4 calcLightProj() const;

	void updateLightViewProj();

public:
	vec3 color;
	vec3 direction;

	AABB sceneBoundingBox;

	mat4 lightViewProj;
};

#endif
