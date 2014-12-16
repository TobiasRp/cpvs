#include "CompressedShadow.h"
#include "MinMaxHierarchy.h"
#include "ShadowMap.h"

#include <cmath>
#include <glm/ext.hpp>
#include <iostream>
using namespace std;

struct AABB {
	AABB(const ivec3& min, const ivec3& max)
		: minPoint(min), maxPoint(max) {
		ivec3 radius;
		radius.x = abs(maxPoint.x - minPoint.x) / 2;
		radius.y = abs(maxPoint.y - minPoint.y) / 2;
		radius.z = abs(maxPoint.z - minPoint.z) / 2;

		midPoint.x = minPoint.x + radius.x;
		midPoint.y = minPoint.y + radius.y;
		midPoint.z = minPoint.z + radius.z;
	}

	ivec3 minPoint, maxPoint, midPoint;

	CompressedShadow::NodeVisibility visible(float min, float max) const {
		if (minPoint.z < min)
			return CompressedShadow::VISIBLE;
		else if (minPoint.z > max)
			return CompressedShadow::SHADOW;
		else
			return CompressedShadow::PARTIAL;
	}

	array<AABB, 8> getChildren() const {
		return array<AABB, 8> {
		// front lower row at the left
		AABB(minPoint, midPoint),

		// front lower row at the right
		AABB(ivec3(midPoint.x, minPoint.y, minPoint.z),
				ivec3(maxPoint.x, midPoint.y, midPoint.z)),

		// front upper row at the left
		AABB(ivec3(minPoint.x, midPoint.y, minPoint.z),
				ivec3(midPoint.x, maxPoint.y, midPoint.z)),

		// front upper row at the right
		AABB(ivec3(midPoint.x, midPoint.y, minPoint.z),
				ivec3(maxPoint.x, maxPoint.y, midPoint.z)),

		// back lower row at the left
		AABB(ivec3(minPoint.x, minPoint.y, midPoint.z),
				ivec3(midPoint.x, midPoint.y, maxPoint.z)),

		// back lower row at the right
		AABB(ivec3(midPoint.x, minPoint.y, midPoint.z),
				ivec3(maxPoint.x, midPoint.y, maxPoint.z)),

		// back upper row at the left
		AABB(ivec3(minPoint.x, midPoint.y, midPoint.z),
				ivec3(midPoint.x, maxPoint.y, maxPoint.z)),

		// back upper row at the right
		AABB(midPoint, maxPoint)
		};
	}

};

CompressedShadow::CompressedShadow(const MinMaxHierarchy& minMax) {
	m_numLevels = minMax.getNumLevels();

	size_t res = getResolution(m_numLevels);
	AABB lightFrustum(ivec3(0, 0, 0), ivec3(res, res, res));
	constructSvoLevel(minMax, m_numLevels - 2, lightFrustum);
}



void CompressedShadow::constructSvoLevel(const MinMaxHierarchy& minMax, size_t level, const AABB& parentNode) {
	auto children = parentNode.getChildren();

	// Test lower left
	children[0].visible(minMax.getMin(level, 0, 0), minMax.getMax(level, 0, 0));
	children[4].visible(minMax.getMin(level, 0, 0), minMax.getMax(level, 0, 0));

	// Test lower right
	children[1].visible(minMax.getMin(level, 1, 0), minMax.getMax(level, 1, 0));
	children[5].visible(minMax.getMin(level, 1, 0), minMax.getMax(level, 1, 0));

	// Test upper left
	children[2].visible(minMax.getMin(level, 0, 1), minMax.getMax(level, 0, 1));
	children[6].visible(minMax.getMin(level, 0, 1), minMax.getMax(level, 0, 1));

	// Test upper right
	children[3].visible(minMax.getMin(level, 1, 1), minMax.getMax(level, 1, 1));
	children[7].visible(minMax.getMin(level, 1, 1), minMax.getMax(level, 1, 1));

}

unique_ptr<CompressedShadow> CompressedShadow::create(const MinMaxHierarchy& minMax) {
	return unique_ptr<CompressedShadow>(new CompressedShadow(minMax));
}

unique_ptr<CompressedShadow> CompressedShadow::create(const ShadowMap& shadowMap) {
	MinMaxHierarchy minMax(shadowMap.createImageF());
	return create(minMax);
}

CompressedShadow::NodeVisibility CompressedShadow::traverse(vec3 position) {
	cout << glm::to_string(getPathFromNDC(position, m_numLevels)) << endl;
	return SHADOW;
}
