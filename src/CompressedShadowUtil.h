#ifndef COMPRESSED_SHADOW_UTIL_H
#define COMPRESSED_SHADOW_UTIL_H

#include "cpvs.h"
#include "MinMaxHierarchy.h"
#include "CompressedShadow.h"

namespace cs {

	/**
	 * Calculates the childmask, i.e. the visibility for every child, for a node given by it's global offset and level.
	 *
	 * @param steps Controls the number of subdivision steps in the x,y and z axis. Thus, steps = 2 will correspond to
	 * an octree. A step size not equal to 2 can be useful in the creation of leafmasks.
	 */
	extern uint64 createChildmask(const MinMaxHierarchy& minMax, size_t level, const ivec3& offset, size_t steps = 2);

	/**
	 * Given the parents childmask and offset, this returns the global offset of the child.
	 * Basically this returns the parents offset + minimum point of the childs AABB.
	 */
	extern vector<ivec3> getChildOffsets(uint childmask, const ivec3& parentOffset);

	/**
	 * Compares a min and max z value and a min/max depth value (probably from the min-max hierarchy)
	 * and returns either visible, shadow or partial
	 */
	inline CompressedShadow::NodeVisibility visible(float minZ, float maxZ, float minDepth, float maxDepth) {
		if (maxZ <= minDepth)
			return CompressedShadow::VISIBLE;
		else if (minZ >= maxDepth)
			return CompressedShadow::SHADOW;
		else {
			return CompressedShadow::PARTIAL;
		}
	}

	/**
	 * Compares a min/max z value and a depth value and returns either visible or shadow, but not partial.
	 * @note Use this function for level 0.
	 */
	inline CompressedShadow::NodeVisibility absoluteVisible(float minZ, float maxZ, float depth) {
		if (maxZ <= depth)
			return CompressedShadow::VISIBLE;
		else
			return CompressedShadow::SHADOW;
	}

	/**
	 * Returns the number of partially visible children in a 16-bit childmask.
	 */
	inline uint getNumChildren(uint childmask) {
		// partialMask will have a bit set for every partially visible child
		uint partialMask = childmask & (0xAAAA);
		// count number of bits set
		return POPCOUNT(partialMask);
	}

	/**
	 * Returns true if the child is partially visible.
	 */
	inline bool isPartial(uint64 childmask, uint childIndex) {
		uint64 mask = 1 << (childIndex * 2 + 1);
		return mask & childmask;
	}

	/**
	 * Returns true if the child is fully visible.
	 */
	inline bool isVisible(uint64 childmask, uint childIndex) {
		uint64 mask = 1 << (childIndex * 2);
		return mask & childmask;
	}

	/**
	 * Returns true if the child is fully in shadow.
	 */
	inline bool isShadowed(uint64 childmask, uint childIndex) {
		return !isVisible(childmask, childIndex) && !isPartial(childmask, childIndex);
	}
};

#endif
