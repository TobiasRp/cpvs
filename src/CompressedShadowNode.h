#ifndef COMPRESSED_SHADOW_NODE_H
#define COMPRESSED_SHADOW_NODE_H

#include "cpvs.h"
#include "MinMaxHierarchy.h"
#include "CompressedShadow.h"

/**
 * Contains helper classes and functions for CompressedShadows, including the Node class.
 */
namespace cs {

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
	 * A CompressedShadow is stored in a linear array with offsets as 'pointers'. But to construct the SVO/DAG
	 * that way would be too complicated.
	 *
	 * Thus the purpose of a Node is to build a temporary SVO/DAG structure, which is only used
	 * during construction and will be transformed to a linear array later.
	 *
	 * @note Note that every node has 8 child pointers, which can be null. These unecessary pointers can also
	 * be removed later on whilst linearizing the DAG.
	 */
	struct Node {
		/** Makes it easier to identify certain child nodes */
		enum NodeNumber {
			FRONT_LOWER_LEFT  = 0,
			FRONT_LOWER_RIGHT = 1,
			FRONT_UPPER_LEFT  = 2,
			FRONT_UPPER_RIGHT = 3,
			BACK_LOWER_LEFT   = 4,
			BACK_LOWER_RIGHT  = 5,
			BACK_UPPER_LEFT   = 6,
			BACK_UPPER_RIGHT  = 7
		};

		uint64           childmask;   // usually only the lower 16-bit are used, but for leafs all 64-bit are needed.
		unique_ptr<Node> children[8]; // Pointers to the children; can be nullptr's
		ivec3            offset;      // Spatial offset (for the minimum point of the node's AABB)
		float            depth;       // Spatial depth (for the z-radius of the node's AABB; x,y are not needed)

		Node()
			: offset(ivec3(0, 0, 0)), depth(2) { }

		Node(const ivec3 off, float d)
			: offset(off), depth(d) { }

		/**
		 * Returns true if the child node is partially visible.
		 */
		inline bool isPartial(NodeNumber nr) {
			uint64 mask = 1 << (nr * 2 + 1);
			return mask & childmask;
		}

		/**
		 * Returns true if the child node is fully visible.
		 */
		inline bool isVisible(NodeNumber nr) {
			uint64 mask = 1 << (nr * 2);
			return mask & childmask;
		}

		/**
		 * Returns true if the child node is fully in shadow.
		 */
		inline bool isShadowed(NodeNumber nr) {
			return !isVisible(nr) && !isPartial(nr);
		}

		/**
		 * Calculates the childmask, it.e. visibility of the children and adds all partially visible children.
		 *
		 * @note The childmask contains 16-bit where every 2-bits are set
		 * to 10, 01 or 00 (partial, visible, shadow).
		 */
		void addChildren(const MinMaxHierarchy& minMax, size_t level, size_t steps = 2);
	};
};

#endif
