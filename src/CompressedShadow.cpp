#include "CompressedShadow.h"
#include "CompressedShadowUtil.h"
#include "MinMaxHierarchy.h"
#include "ShadowMap.h"

#include <algorithm>
#include <cmath>
using namespace cs;
using namespace std;

// for cout testing and debugging!
#include <glm/ext.hpp>
#include <iostream>

constexpr uint NODE_SIZE = 9; // childmask + 8 pointers (unused pointer will be removed later)

CompressedShadow::CompressedShadow(const MinMaxHierarchy& minMax) {
	m_numLevels = minMax.getNumLevels();
	assert(m_numLevels > 3);

	constructSvo(minMax);
}

/**
 * Given a node through it's offset and a pointer to the beginning of it's children, this helper
 * function sets the nodes pointers to it's children in the dag.
 */
void setChildrenOffsets(vector<uint>& dag, size_t nodeOffset, size_t childrenOffset, uint numChildren) {
	for (uint i = 0; i < numChildren; ++i) {
		dag[nodeOffset + 1 + i] = childrenOffset + i * NODE_SIZE;
	}
}

void CompressedShadow::constructSvo(const MinMaxHierarchy& minMax) {
	const ivec3 rootOffset(0, 0, 0);
	uint64 rootmask  = cs::createChildmask(minMax, m_numLevels - 2, rootOffset);
	uint numChildren = cs::getNumChildren(rootmask);
 
	// Resize so root and children fit in
	m_dag.resize(NODE_SIZE + numChildren * NODE_SIZE);
	m_dag[0] = rootmask;

	vector<ivec3> childCoords = cs::getChildOffsets(rootmask, rootOffset);
	setChildrenOffsets(m_dag, 0, NODE_SIZE, numChildren);

	uint levelOffset     = NODE_SIZE;
	size_t numLevelNodes = numChildren;

	vector<ivec3> newChildrenCoords;

	int level = m_numLevels - 3;

	/* Create new levels from the highest to the lowest level */
	while(level >= 0 && numLevelNodes > 0) {
		size_t nextLevelOffset   = levelOffset + numLevelNodes * NODE_SIZE;
		size_t newChildrenNodes  = 0;
		size_t nextLevelProgress = 0;
		newChildrenCoords.clear();

		/* First calculate the number of new children nodes so we can resize the dag.
		 * Thereby save all masks so we don't have to calculate them twice */
		vector<uint64> masks(numLevelNodes);
		for (size_t nodeNr = 0; nodeNr < numLevelNodes; ++nodeNr) {
			uint64 nodemask = cs::createChildmask(minMax, level, childCoords[nodeNr]);
			numChildren     = cs::getNumChildren(nodemask);

			masks[nodeNr] = nodemask;
			newChildrenNodes += numChildren;
		}

		if (level != 0)
			m_dag.resize(m_dag.size() + newChildrenNodes * NODE_SIZE, 0);

		for (size_t nodeNr = 0; nodeNr < numLevelNodes; ++nodeNr) {
			size_t nodeOffset   = levelOffset + nodeNr * NODE_SIZE;
			uint64 nodemask   = masks[nodeNr];
			numChildren       = cs::getNumChildren(nodemask);
			m_dag[nodeOffset] = nodemask;

			if (numChildren > 0) {
				assert(level != 0);

				auto coords = cs::getChildOffsets(nodemask, childCoords[nodeNr]);
				size_t childOffset = nextLevelOffset + nextLevelProgress;

				setChildrenOffsets(m_dag, nodeOffset, childOffset, numChildren);
				newChildrenCoords.insert(newChildrenCoords.end(), coords.begin(), coords.end());

				nextLevelProgress += numChildren * NODE_SIZE;
			}
		}

		levelOffset += numLevelNodes * NODE_SIZE;
		
		numLevelNodes = newChildrenNodes;
		childCoords   = newChildrenCoords;

		level--;
	}
}

unique_ptr<CompressedShadow> CompressedShadow::create(const MinMaxHierarchy& minMax) {
	return unique_ptr<CompressedShadow>(new CompressedShadow(minMax));
}

unique_ptr<CompressedShadow> CompressedShadow::create(const ShadowMap& shadowMap) {
	MinMaxHierarchy minMax(shadowMap.createImageF());
	return create(minMax);
}

CompressedShadow::NodeVisibility CompressedShadow::traverse(const vec3 position) {
	const ivec3 path = getPathFromNDC(std::move(position), m_numLevels);

	size_t offset = 0;
	int level     = m_numLevels;
	while(level > 1) {
		int lvlBit = 1 << (level - 2);
		int childIndex = ((path.x & lvlBit) ? 1 : 0) +
		                 ((path.y & lvlBit) ? 2 : 0) +
						 ((path.z & lvlBit) ? 4 : 0);

		uint64 childmask = m_dag[offset];

		if(isVisible(childmask, childIndex)) {
			return VISIBLE;
		} else if (isShadowed(childmask, childIndex)) {
			return SHADOW;
		} else {
			// We need the child index counting only partially visible children

			uint childBits = childIndex * 2;
			// 0xAAAA is a mask for partial visibility
			uint maskedChildMask = childmask & (0xAAAA >> (16 - childBits));

			// Count the bits set, this is the correct offset
			uint childOffset = POPCOUNT(maskedChildMask);

			offset = m_dag[offset + 1 + childOffset];
		}

		level -= 1;
	}

	return PARTIAL;
}
