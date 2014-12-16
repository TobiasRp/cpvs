#include "CompressedShadowNode.h"

/* For cout debugging :-) */
#include <iostream>
#include <glm/ext.hpp>

using namespace std;
using namespace cs;

array<AABB, 8> cs::AABB::findChildren() const {
	ivec3 midPoint;
	midPoint.x = minPoint.x + ceil(abs(maxPoint.x - minPoint.x) / 2);
	midPoint.y = minPoint.y + ceil(abs(maxPoint.y - minPoint.y) / 2);
	midPoint.z = minPoint.z + ceil(abs(maxPoint.z - minPoint.z) / 2);

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

void cs::Node::calcChildmask(const MinMaxHierarchy& minMax, const array<AABB, 8>& children,
		size_t level, size_t parX, size_t parY) {

	/* The y-axis of the image in the min-max hierarchy is inverted */
	size_t levelHeight = minMax.getLevel(level)->getHeight(); 
	size_t offY = levelHeight - parY - 1;

	childmask = 0;
	size_t size = CompressedShadow::getResolution(minMax.getNumLevels());

	// Test lower left
	auto min = minMax.getMin(level, parX, offY);
	auto max = minMax.getMax(level, parX, offY);
	auto c000 = children[FRONT_LOWER_LEFT].visible(size, min, max);
	auto c001 = children[BACK_LOWER_LEFT].visible(size, min, max);
	childmask |= c000 << (FRONT_LOWER_LEFT * 2);
	childmask |= c001 << (BACK_LOWER_LEFT * 2);

	// Test upper left
	min = minMax.getMin(level, parX, offY - 1);
	max = minMax.getMax(level, parX, offY - 1);
	
	auto c010 = children[FRONT_UPPER_LEFT].visible(size, min, max);
	auto c011 = children[BACK_UPPER_LEFT].visible(size, min, max);
	childmask |= c010 << (FRONT_UPPER_LEFT * 2);
	childmask |= c011 << (BACK_UPPER_LEFT * 2);

	// Test lower right
	min = minMax.getMin(level, parX + 1, offY);
	max = minMax.getMax(level, parX + 1, offY);
	auto c100 = children[FRONT_LOWER_RIGHT].visible(size, min, max);
	auto c101 = children[BACK_LOWER_RIGHT].visible(size, min, max);
	childmask |= c100 << (FRONT_LOWER_RIGHT * 2);
	childmask |= c101 << (BACK_LOWER_RIGHT * 2);

	// Test upper right
	min = minMax.getMin(level, parX + 1, offY - 1);
	max = minMax.getMax(level, parX + 1, offY - 1);
	auto c110 = children[FRONT_UPPER_RIGHT].visible(size, min, max);
	auto c111 = children[BACK_UPPER_RIGHT].visible(size, min, max);
	childmask |= c110 << (FRONT_UPPER_RIGHT * 2);
	childmask |= c111 << (BACK_UPPER_RIGHT * 2);
}

void cs::Node::addNewChildren(const array<AABB, 8>& aabbs) {
	for (uint i = 0; i < 8; ++i) {
		if (isPartial(static_cast<NodeNumber>(i))) {
			children[i] = make_unique<Node>(aabbs[i]);
		} else {
			children[i] = nullptr;
		}
	}

}
