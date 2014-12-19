#include "CompressedShadowUtil.h"

/* For cout debugging :-) */
#include <iostream>
#include <glm/ext.hpp>

using namespace std;
using namespace cs;

uint64 cs::createChildmask(const MinMaxHierarchy& minMax, size_t level, const ivec3& offset, size_t steps) {
	assert(steps == 2 && "Haven't tested/implemented with steps != 2");

	/* Size of the minMax level (levelHeight) */
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

vector<ivec3> cs::getChildOffsets(uint childmask, const ivec3& parentOffset) {
	vector<ivec3> result;
	for (uint i = 0; i < 8; ++i) {
		if (isPartial(childmask, i)) {
			uint maskX = (1 & i) ? 1 : 0;
			uint maskY = (2 & i) ? 1 : 0;
			uint maskZ = (4 & i) ? 1 : 0;
			ivec3 off((parentOffset.x + maskX) * 2, (parentOffset.y + maskY) * 2, (parentOffset.z + maskZ) * 2);

			result.push_back(off);
		}
	}
	return result;
}
