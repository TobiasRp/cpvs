#ifndef COMPRESSED_SHADOW_UTIL_H
#define COMPRESSED_SHADOW_UTIL_H

#include "cpvs.h"
#include "MinMaxHierarchy.h"
#include "CompressedShadow.h"

namespace cs {
	constexpr uint NODE_SIZE = 9; // childmask + 8 pointers (unused pointers will be removed with 'compress')
	constexpr uint LEAF_SIZE = 17; // childmask + 8 64-bit leafmask

	/**
	 * Calculates the childmask, i.e. the visibility for every child, for a node given by it's global offset and level.
	 */
	extern uint createChildmask(const MinMaxHierarchy& minMax, uint level, const ivec3& offset);

	/**
	 * Calculates a 64-bit leafmask which stores 4x4x4 visibility values (visible, shadow but not partial)
	 */
	extern uint64 createLeafmask(const MinMaxHierarchy& minMax, const ivec3& offset);

	/**
	 * Given the parents childmask and coordinates, this returns the coordinates of all partially visible children.
	 * Basically this returns the parents offset + minimum point of the childs AABB for all children.
	 */
	extern vector<ivec3> getChildCoordinates(uint childmask, const ivec3& parentOffset);

	/**
	 * Compares a min and max z-coordinate and a min/max depth value (probably from the min-max hierarchy)
	 * and returns either visible, shadow or partial.
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
		// partialMask will have 0x2 set for every partially visible child
		uint partialMask = childmask & (0xAAAA);
		// count number of bits set
		return POPCOUNT(partialMask);
	}

	/**
	 * Returns true if the child is partially visible.
	 * \param childIndex The number of the child, i.e. in the range [0-7]
	 */
	inline bool isPartial(uint64 childmask, uint childIndex) {
		uint64 mask = 1 << (childIndex * 2 + 1);
		return mask & childmask;
	}

	/**
	 * Returns true if the child is fully visible.
	 * \param childIndex The number of the child, i.e. in the range [0-7]
	 */
	inline bool isVisible(uint64 childmask, uint childIndex) {
		uint64 mask = 1 << (childIndex * 2);
		return mask & childmask;
	}

	/**
	 * Returns true if the child is fully in shadow.
	 * \param childIndex The number of the child, i.e. in the range [0-7]
	 */
	inline bool isShadowed(uint64 childmask, uint childIndex) {
		return !isVisible(childmask, childIndex) && !isPartial(childmask, childIndex);
	}

	/**
	 * Compares two nodes and decides whether they are identical subtrees and can thus be merged.
	 */
	template<typename It1, typename It2>
	inline bool isEqualSubtree(It1 leftNode, It2 rightNode, uint nodeSize) {
		for (uint i = 0; i < nodeSize; ++i, ++leftNode, ++rightNode) {
			/* Compare childmask and all pointers for equality */
			if (*leftNode != *rightNode)
				return false;
		}
		return true;
	}

	/**
	 * Merges all identical subtrees in one level and writes them to ItNew.
	 * @result Maps the old offsets to the nodes to the new ones, so the parents can be updated.
	 *
	 * @note The mapping is in the range [0, number of nodes * NODE_SIZE); you may need
	 * to add the level offset.
	 */
	template<typename ItOld, typename ItNew>
	unordered_map<uint, uint> mergeLevel(ItOld oldBegin, ItOld oldEnd, ItNew newBegin, uint nodeSize,
			uint* numNodesLeft) {
		unordered_map<uint, uint> result;

		ItNew newCurrent = newBegin;
		ItNew newPos; // for finding merging opportunities, i.e. will be repeatedly iterated up to newCurrent
		uint pos; // similar to newPos but contains an offset in the level for creating the result map
		
		ItOld oldCurrent = oldBegin;
		for (uint i = 0; oldCurrent != oldEnd; oldCurrent += nodeSize, i += nodeSize) {

			for (pos = 0, newPos = newBegin; newPos != newCurrent; newPos += nodeSize, pos += nodeSize) {
				if (isEqualSubtree(oldCurrent, newPos, nodeSize)) {
					break;
				}
			}
			result[i] = pos;

			if (newPos == newCurrent) {
				// Insert the node since it can't be merged
				std::copy(oldCurrent, oldCurrent + nodeSize, newCurrent);
				newCurrent += nodeSize;
			}
		}
		*numNodesLeft = std::distance(newBegin, newCurrent) / nodeSize;

		return result;
	}
};

#endif
