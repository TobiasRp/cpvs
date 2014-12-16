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
	 * Axis-aligned bounding box.
	 *
	 * An AABB is defined by a minimum and a maximum point.
	 */
	struct AABB {
		ivec3 minPoint, maxPoint;

		AABB(const ivec3& min, const ivec3& max)
			: minPoint(min), maxPoint(max) { }

		/**
		 * Decides if this AABB is visible, shadowed or partially visible.
		 * @param min Minimum depth value (from the min-max hierarchy), a value between [0, 1]
		 * @param max Analog to min
		 * @param resolution Resolution of the total volume.
		 */
		inline CompressedShadow::NodeVisibility visible(size_t resolution, float min, float max) const {
			const float maxDepth = maxPoint.z;
			const float minDepth = minPoint.z;

			if (maxDepth <= min * resolution)
				return CompressedShadow::VISIBLE;
			else if (minDepth >= max * resolution)
				return CompressedShadow::SHADOW;
			else
				return CompressedShadow::PARTIAL;
		}

		/**
		 * Calculates 8-children from an AABB.
		 */
		array<AABB, 8> findChildren() const;

		inline void doubleSize() {
			minPoint *= 2;
			maxPoint *= 2;
		}
	};

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

		uint             childmask;
		unique_ptr<Node> children[8];
		AABB       aabb;

		Node(const AABB& bb)
			: aabb(bb) { }

		Node(AABB&& bb)
			: aabb(std::move(bb)) { }

		/**
		 * Returns true if the child node is partially visible.
		 */
		inline bool isPartial(NodeNumber nr) {
			uint mask = 1 << (nr * 2 + 1);
			return mask & childmask;
		}

		/**
		 * Returns true if the child node is fully visible.
		 */
		inline bool isVisible(NodeNumber nr) {
			uint mask = 1 << (nr * 2);
			return mask & childmask;
		}

		/**
		 * Returns true if the child node is fully in shadow.
		 */
		inline bool isShadowed(NodeNumber nr) {
			return !isVisible(nr) && !isPartial(nr);
		}

		/**
		 * Calculates the childmask, i.e. the visibility of the given eight AABBs.
		 * parX and parY are the x- and y-offset into the min-max hierarchy.
		 *
		 * @param children As returned from AABB::findChildren
		 *
		 * @return 16-bit childmask where every 2-bits are set to 10, 01 or 00 (partial, visible, shadow).
		 */
		void calcChildmask(const MinMaxHierarchy& minMax, const array<AABB, 8>& children, 
				size_t level, size_t parX, size_t parY);

		/**
		 * Adds all children which are of partial visibility to the Node.
		 *
		 * @param children As returned from AABB::findChildren
		 *
		 * @note This assumes that the childmask is calculated and correct.
		 */
		void addNewChildren(const array<AABB, 8>& aabbs);

		/**
		 * Doubles the size of the AABB so it can be used on the next level
		 * in the hierarchy with a bigger resolution.
		 */
		inline void doubleAABB() {
			aabb.doubleSize();
		}
	};
};

#endif
