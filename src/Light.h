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

	/**
	 * Calculates the light projection for the sub-frustum at (x, y, z)
	 * with x, y, z in [0, resolution)
	 */
	mat4 calcLightProj(uint x, uint y, uint z, uint resolution) const;

	void updateLightViewProj();

private:
	void getMinMaxValues(float* minX, float* minY, float* maxX, float* maxY, float* zfar) const;

public:
	vec3 color;
	vec3 direction;

	AABB sceneBoundingBox;

	mat4 lightViewProj;
};

#endif
