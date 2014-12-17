#include "CompressedShadowNode.h"

/* For cout debugging :-) */
#include <iostream>
#include <glm/ext.hpp>

using namespace std;
using namespace cs;

void cs::Node::addChildren(const MinMaxHierarchy& minMax, size_t level, size_t steps) {
	assert(steps == 2 && "Haven't tested/implemented with steps != 2");

	/* Size of the minMax level (levelHeight) and size of the total resolution of the volume (size) */
	size_t levelHeight = minMax.getLevel(level)->getHeight();
//	size_t size = CompressedShadow::getResolution(minMax.getNumLevels());

	/* The y-axis of the image in the min-max hierarchy is inverted */
	size_t invertedY = levelHeight - offset.y - 1;

	childmask = 0;

	cout << "depth " << depth << endl;

	for (uint z = 0; z < steps; ++z) {
		for (uint y = 0; y < steps; ++y) {
			for (uint x = 0; x < steps; ++x) {
				float depthHalf = depth / 2.0f;
				size_t offZ = z * depthHalf + offset.z;
				size_t offY = invertedY - y;
				size_t offX = x + offset.x;

				auto min = minMax.getMin(level, offX, offY);
				auto max = minMax.getMax(level, offX, offY);

				cout << "minZ = " << offZ << " maxZ = " << offZ + depthHalf << " min*s = " << min * levelHeight << " max*s = " << max * levelHeight << endl;
				auto bits = visible(offZ, offZ + depthHalf, min * levelHeight, max * levelHeight);

				/* Combine x, y, z to get the index of the child we just calculated the visibility for */
				uint childNr = x;
				childNr |= y << (steps / 2);
				childNr |= z << steps;

				childmask |= bits << (childNr * 2);

				if (bits == CompressedShadow::PARTIAL)
					children[childNr] = make_unique<Node>(ivec3(offX, offset.y + y, offZ), depth);
			}
		}
	}
}
