#include "CompressedShadow.h"
#include "CompressedShadowUtil.h"
#include "MinMaxHierarchy.h"
#include "ShadowMap.h"

#include <thread>
#include <algorithm>
#include <cmath>
using namespace cs;
using namespace std;

// for testing and debugging
#include <glm/ext.hpp>
#include <iostream>

// Some constants
constexpr uint NODE_SIZE = 9; // childmask + 8 pointers (can be compressed later)

CompressedShadow::CompressedShadow(const MinMaxHierarchy& minMax) {
	m_numLevels = minMax.getNumLevels();
	assert(m_numLevels > 3);

	constructSvo(minMax);
}

void CompressedShadow::constructSvo(const MinMaxHierarchy& minMax) {
	const ivec3 rootOffset(0, 0, 0);
	uint64 rootmask = cs::createChildmask(minMax, m_numLevels - 2, rootOffset);
	uint numChildren = cs::getNumChildren(rootmask);

	m_dag.resize(NODE_SIZE + numChildren * NODE_SIZE); // resize so root + children fit in
	m_dag[0] = rootmask;

	vector<ivec3> childCoords = cs::getChildOffsets(rootmask, rootOffset);
	setChildrenOffsets(0, NODE_SIZE, numChildren);

	uint levelOffset = NODE_SIZE;// + numChildren * NODE_SIZE;
	size_t numLevelNodes = numChildren;

	vector<ivec3> newChildrenCoords;

	int level = m_numLevels - 3;
	while(level >= 0 && numLevelNodes > 0) {
		size_t nextLevelOffset  = levelOffset + numLevelNodes * NODE_SIZE;

		size_t newChildrenNodes = 0;
		newChildrenCoords.clear();

		/* First calculates the number of new children nodes so we can resize the dag.
		 * Thereby save all masks so we don't have to calculate them twice */
		vector<uint64> masks(numLevelNodes);
		for (size_t nodeNr = 0; nodeNr < numLevelNodes; ++nodeNr) {
//			cout << glm::to_string(childCoords[nodeNr]) << endl;
			uint64 nodemask = cs::createChildmask(minMax, level, childCoords[nodeNr]);
			numChildren     = cs::getNumChildren(nodemask);

			masks[nodeNr] = nodemask;
//			cout << "nodeNr " << nodeNr << " with mask " << hex << nodemask << endl;
			newChildrenNodes += numChildren;
		}

		if (level != 0)
			m_dag.resize(m_dag.size() + newChildrenNodes * NODE_SIZE, 0);

		for (size_t nodeNr = 0; nodeNr < numLevelNodes; ++nodeNr) {
			uint nodeOffset   = levelOffset + nodeNr * NODE_SIZE;
			uint64 nodemask   = masks[nodeNr];
			numChildren       = cs::getNumChildren(nodemask);
			m_dag[nodeOffset] = nodemask;

			if (numChildren > 0) {
				assert(level != 0);

				auto coords = cs::getChildOffsets(nodemask, childCoords[nodeNr]);

				setChildrenOffsets(nodeOffset, nextLevelOffset + nodeNr * NODE_SIZE, numChildren);
				newChildrenCoords.insert(newChildrenCoords.end(), coords.begin(), coords.end());
			}
		}

		levelOffset += numLevelNodes * NODE_SIZE;
		
		numLevelNodes = newChildrenNodes;
		childCoords   = newChildrenCoords;

		level--;
	}

	for (uint i = 0; i < m_dag.size(); ++i) {
		cout << hex << m_dag[i] << ", ";
	}
}

void CompressedShadow::setChildrenOffsets(size_t nodeOffset, size_t childrenOffset, uint numChildren) {
	for (uint i = 0; i < numChildren; ++i) {
		m_dag[nodeOffset + 1 + i] = childrenOffset + i * NODE_SIZE;
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
	ivec3 path = getPathFromNDC(std::move(position), m_numLevels);

	//cout << "path = " << glm::to_string(path) << endl;

	size_t offset = 0;

	int level = m_numLevels;
	while(level > 1) {
		//cout << "leveeel is " << level << endl;
		int lvlBit = 1 << (level - 2);
		//cout << "lvlBit " << lvlBit << endl;
		int childIndex = ((path.x & lvlBit) ? 1 : 0) +
		                 ((path.y & lvlBit) ? 2 : 0) +
						 ((path.z & lvlBit) ? 4 : 0);

		//cout << "offset is " << offset << " and ci = " << childIndex << endl;
		uint64 childmask = m_dag[offset];

		//cout << hex << childmask << endl;

		if(isVisible(childmask, childIndex)) {
			return VISIBLE;
		} else if (isShadowed(childmask, childIndex)) {
			return SHADOW;
		} else {
			//FIXME
			uint maskedChildMask = childmask & (0xAAAA >> (8-childIndex));
			uint childOffset = __builtin_popcount(maskedChildMask);
			cout << childOffset << endl;
			offset = m_dag[offset + 1 + childOffset];
		}

		level -= 1;
	}

	return PARTIAL;
}
