#include "CompressedShadowNode.h"

/* For cout debugging :-) */
#include <iostream>
#include <glm/ext.hpp>

using namespace std;
using namespace cs;

void cs::Node::addChildren(const MinMaxHierarchy& minMax, size_t level, size_t steps) {
	assert(steps == 2 && "Haven't tested/implemented with steps != 2");

	/* Size of the minMax level (levelHeight) */
	size_t levelHeight = minMax.getLevel(level)->getHeight();

	/* The y-axis of the image in the min-max hierarchy is inverted */
	size_t invertedY = levelHeight - offset.y - 1;

	childmask = 0;
	for (uint z = 0; z < steps; ++z) {
		for (uint y = 0; y < steps; ++y) {
			for (uint x = 0; x < steps; ++x) {
				float depthHalf = depth / 2.0f;
				size_t offZ = z * depthHalf + offset.z;
				size_t offY = invertedY - y;
				size_t offX = x + offset.x;

				auto min = minMax.getMin(level, offX, offY);
				auto max = minMax.getMax(level, offX, offY);

				uint bits;
				if (level > 0) {
					bits = visible(offZ, offZ + depthHalf, min * levelHeight, max * levelHeight);
				} else {
					// At level 0 we want to be sure about the visibility
					bits = absoluteVisible(offZ, offZ + depthHalf, min * levelHeight);
				}

				/* Combine x, y, z to get the index of the child we just calculated the visibility for */
				uint childNr = x;
				childNr |= y << (steps / 2);
				childNr |= z << steps;

				// Now set the two bits we've just calculated
				childmask |= bits << (childNr * 2);

				if (bits == CompressedShadow::PARTIAL) {
					ivec3 newOffset = ivec3(offX * 2, (offset.y + y) * 2, offZ * 2);
					children[childNr] = make_unique<Node>(newOffset, depth);
				}
			}
		}
	}
}
