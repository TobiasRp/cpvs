#include "CompressedShadowUtil.h"

/* For cout debugging :-) */
#include <iostream>
#include <glm/ext.hpp>

using namespace std;
using namespace cs;

uint64 cs::createChildmask(const MinMaxHierarchy& minMax, size_t level, const ivec3& offset, size_t steps) {
	assert(steps == 2 && "Haven't tested/implemented with steps != 2");

	/* Size of the minMax level */
	size_t levelHeight = minMax.getLevel(level)->getHeight();

	/* The y-axis of the image in the min-max hierarchy is inverted */
	size_t invertedY = levelHeight - offset.y - 1;

	uint64 childmask = 0;
	for (uint z = 0; z < steps; ++z) {
		for (uint y = 0; y < steps; ++y) {
			for (uint x = 0; x < steps; ++x) {
				float depthHalf = 2.0f / static_cast<float>(steps);
				float offZ = z * depthHalf + offset.z;
				size_t offY = invertedY - y;
				size_t offX = x + offset.x;

				auto min = minMax.getMin(level, offX, offY);
				auto max = minMax.getMax(level, offX, offY);

				uint64 bits;
				if (level > 0) {
					bits = visible(offZ, offZ + depthHalf, min * levelHeight, max * levelHeight);
				} else {
					// At level 0 we want to be sure about the visibility
					assert(abs(min - max) < 1e-6f);
					bits = absoluteVisible(offZ, offZ + depthHalf, min * levelHeight);
				}

				/* Combine x, y, z to get the index of the child we just calculated the visibility for */
				uint64 childNr = x;
				childNr |= y << (steps / 2);
				childNr |= z << steps;

				/* Now set the two bits we've just calculated at the right position */
				childmask |= bits << (childNr * 2);
			}
		}
	}
	return childmask;
}

vector<ivec3> cs::getChildCoordinates(uint childmask, const ivec3& parentOffset) {
	vector<ivec3> result;
	for (uint i = 0; i < 8; ++i) {
		if (isPartial(childmask, i)) {
			/* Decide whether to add 1 in the x, y, z direction.
			 * This code relies on the order of the children (and has to...) */

			uint maskX = (0x1 & i) ? 1 : 0; // maskX is 1 <=> i is 1, 3, 5, 7
			uint maskY = (0x2 & i) ? 1 : 0; // maskY is 1 <=> i is 2, 3, 6, 7
			uint maskZ = (0x4 & i) ? 1 : 0; // maskZ is 1 <=> i is 4, 5, 6, 7
			ivec3 off((parentOffset.x + maskX) * 2, (parentOffset.y + maskY) * 2, (parentOffset.z + maskZ) * 2);

			result.push_back(off);
		}
	}
	return result;
}
