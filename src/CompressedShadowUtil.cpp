#include "CompressedShadowUtil.h"

/* For cout debugging :-) */
#include <iostream>
#include <glm/ext.hpp>

using namespace std;
using namespace cs;

static uint depthOffset = 1;

void cs::setDepthOffset(uint off) {
	depthOffset = off;
}

inline uint getLevelHeight(const MinMaxHierarchy& minMax, uint level) {
	return minMax.getLevel(level)->getHeight() * depthOffset;
}

uint cs::createChildmask(const MinMaxHierarchy& minMax, uint level, const ivec3& offset) {
	auto levelHeight = getLevelHeight(minMax, level);

	uint childmask = 0;
	for (uint z = 0; z < 2; ++z) {
		for (uint y = 0; y < 2; ++y) {
			for (uint x = 0; x < 2; ++x) {
				uint offZ = z + offset.z;
				uint offY = y + offset.y;
				uint offX = x + offset.x;

				auto min = minMax.getMin(level, offX, offY);
				auto max = minMax.getMax(level, offX, offY);

				uint bits;
				if (level > 0) {
					bits = visible(offZ, offZ + 1, min * levelHeight, max * levelHeight);
				} else {
					// At level 0 we want to be sure about the visibility
					assert(abs(min - max) < 1e-6f);
					bits = absoluteVisible(offZ, offZ + 1, min * levelHeight);
				}

				/* Combine x, y, z to get the index of the child we just calculated the visibility for */
				uint childNr = x;
				childNr |= y << 1;
				childNr |= z << 2;

				/* Now set the two bits we've just calculated at the right position */
				childmask |= bits << (childNr * 2);
			}
		}
	}
	return childmask;
}

/**
 * Calculates a 64-bit leafmask which stores 8x8x1 visibility values (visible, shadow but not partial).
 */
inline uint64 createLeafmask(const MinMaxHierarchy& minMax, const ivec3& offset) {
	auto levelHeight = getLevelHeight(minMax, 0);

	uint64 leafmask = 0;
	uint index = 0;
	for (uint y = 0; y < 8; ++y) {
		float offY = offset.y + y;

		for (uint x = 0; x < 8; ++x) {
			float offX = offset.x + x;

			auto min = minMax.getMin(0, offX, offY);
			uint64 bit = absoluteVisible(offset.z, offset.z + 1, min * levelHeight);
			leafmask |= bit << index;

			++index;
		}
	}
	return leafmask;
}

std::pair<uint, vector<uint64>> cs::createChildmask1x1x8(const MinMaxHierarchy& minMax, const ivec3& offset) {
	ivec3 offCorrected = offset * 4;

	uint childmask = 0;
	vector<uint64> masks;
	for (uint z = 0; z < 8; ++z) {
		uint64 leafmask = createLeafmask(minMax, offCorrected);
		++offCorrected.z;

		if (leafmask == 0xFFFFFFFFFFFFFFFF)
			childmask |= 1 << (z * 2);
		else if (leafmask == 0x0)
			childmask |= 0 << (z * 2);
		else {
			childmask |= 0x2 << (z * 2);
			masks.push_back(leafmask);
		}
	}
	return make_pair(childmask, masks);
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

			result.emplace_back(ivec3((parentOffset.x + maskX) * 2,
						(parentOffset.y + maskY) * 2, (parentOffset.z + maskZ) * 2));
		}
	}
	return result;
}
