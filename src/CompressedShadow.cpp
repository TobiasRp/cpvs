#include "CompressedShadow.h"
#include "CompressedShadowUtil.h"
#include "MinMaxHierarchy.h"
#include "ShadowMap.h"

#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <limits>
using namespace cs;
using namespace std;

#include <glm/ext.hpp>
#include <iostream>

// Enable/disable leafmasks. Also has to be modified in traversal.cs
#define LEAFMASKS

/* Decides whether to use 64-bit leafmaks */
inline bool useLeafmasks(uint numLevels) {
#ifdef LEAFMASKS
	// Use leafmasks except when there aren't enough levels
	return (numLevels - 3) >= 2;
#else
	return false;
#endif
}

/* Returns the minimum level (which is not 0 when leafmasks are used) */
inline uint getMinLevel(uint m_numLevels) {
	return useLeafmasks(m_numLevels) ? 2 : 0;
}

/* Returns the size of a node, which can be bigger when leafmasks are used */
inline uint getNodeSize(uint m_numLevels, uint level) {
	bool leafs = useLeafmasks(m_numLevels);
	if (leafs && level == 2)
		return LEAF_SIZE;
	else
		return NODE_SIZE;
}

CompressedShadow::CompressedShadow(uint numLevels)
	: m_numLevels(numLevels)
{
	assert(m_numLevels > 3);
}

unique_ptr<CompressedShadow> CompressedShadow::create(const MinMaxHierarchy& minMax,
		uint zTileIndex, uint zTileNum) {
	auto cs = unique_ptr<CompressedShadow>(new CompressedShadow(minMax.getNumLevels()));

	cs::setDepthOffset(zTileNum);
	auto levels = cs->constructSvo(minMax, ivec3(0, 0, zTileIndex * 2));
	cs->mergeCommonSubtrees(levels);
	cs->compress();

	return cs;
}

unique_ptr<CompressedShadow> CompressedShadow::create(const ShadowMap* shadowMap, uint zTileIndex, uint zTileNum) {
	MinMaxHierarchy minMax(shadowMap->createImageF());
	return create(minMax, zTileIndex, zTileNum);
}

CompressedShadow::NodeVisibility CompressedShadow::getTotalVisibility() const {
	if (isCompletelyVisible(m_dag[0]))
		return VISIBLE;
	else if (isCompletelyShadowed(m_dag[0]))
		return SHADOW;
	return PARTIAL;
}

/**
 * Given a node through it's offset and a pointer to the beginning of it's children, this helper
 * function sets the nodes pointers to it's children in the dag.
 */
inline void setChildrenOffsets(vector<uint>& dag, size_t nodeOffset, size_t childrenOffset, uint numChildren,
		uint nodeSize) {
	assert(childrenOffset + numChildren * nodeSize < std::numeric_limits<uint>::max());

	for (uint i = 0; i < numChildren; ++i) {
		dag[nodeOffset + 1 + i] = childrenOffset + i * nodeSize;
	}
}

vector<uint> CompressedShadow::constructSvo(const MinMaxHierarchy& minMax, const ivec3 rootOffset) {
	uint rootmask  = cs::createChildmask(minMax, m_numLevels - 2, rootOffset);
	uint numChildren = cs::getNumChildren(rootmask);
 
	// Resize so root and children fit in
	m_dag.resize(NODE_SIZE + numChildren * getNodeSize(m_numLevels, m_numLevels - 3));
	m_dag[0] = rootmask;

	vector<ivec3> childCoords = cs::getChildCoordinates(rootmask, rootOffset);
	setChildrenOffsets(m_dag, 0, NODE_SIZE, numChildren, getNodeSize(m_numLevels, m_numLevels - 3));

	size_t levelOffset   = NODE_SIZE;   // Offset to the beginning of the current level
	size_t numLevelNodes = numChildren; // Number of nodes in the current level

	/* Save level offsets so we can later traverse bottom up efficiently */
	vector<uint> levelOffsets(m_numLevels - 1, 0);
	levelOffsets[m_numLevels - 2] = 0;

	int level = m_numLevels - 3;
	int lastLevel = useLeafmasks(m_numLevels) ? 3 : 0; // for leafmasks stop at level 3 and do level 2 seperately

	/* Create new levels from the highest to the lowest level */
	while(level >= lastLevel && numLevelNodes > 0) {
		levelOffsets[level] = levelOffset;

		/* We know that the current node are of size NODE_SIZE, but the children could be leafs.
		 * So always use childNodeSize if the size of a child node is needed. */
		const uint childNodeSize = getNodeSize(m_numLevels, level - 1);

		// Offset to the beginning of the next level
		size_t nextLevelOffset = levelOffset + numLevelNodes * NODE_SIZE;

		size_t newChildrenNodes  = 0; // counts the number of new children in the next level
		size_t nextLevelProgress = 0; // current index in the next level
		vector<ivec3> newChildrenCoords;

		/* First calculate the number of new children nodes so we can resize the dag.
		 * Thereby set and save all masks so we don't have to calculate them twice */
		for (size_t nodeNr = 0; nodeNr < numLevelNodes; ++nodeNr) {
			size_t nodeOffset = levelOffset + nodeNr * NODE_SIZE;
			uint nodemask     = cs::createChildmask(minMax, level, childCoords[nodeNr]);
			numChildren       = cs::getNumChildren(nodemask);

			m_dag[nodeOffset] = nodemask;
			newChildrenNodes += numChildren;
		}

		if (level != 0)
			m_dag.resize(m_dag.size() + newChildrenNodes * childNodeSize, 0);

		for (size_t nodeNr = 0; nodeNr < numLevelNodes; ++nodeNr) {
			size_t nodeOffset = levelOffset + nodeNr * NODE_SIZE;
			uint nodemask     = m_dag[nodeOffset];
			numChildren       = cs::getNumChildren(nodemask);

			if (numChildren > 0) {
				auto coords = cs::getChildCoordinates(nodemask, childCoords[nodeNr]);
				size_t childOffset = nextLevelOffset + nextLevelProgress;

				setChildrenOffsets(m_dag, nodeOffset, childOffset, numChildren, childNodeSize);
				newChildrenCoords.insert(newChildrenCoords.end(), coords.begin(), coords.end());

				nextLevelProgress += numChildren * childNodeSize;
			}
		}

		levelOffset += numLevelNodes * NODE_SIZE;
		
		numLevelNodes = newChildrenNodes;
		childCoords.swap(newChildrenCoords);

		level--;
	}

	/* Construct leaf nodes separately if leafmasks are used */
	if (useLeafmasks(m_numLevels) && level == 2) {
		levelOffsets[level]     = levelOffset;
		levelOffsets[level - 1] = m_dag.size();

		constructLastLevels(minMax, levelOffset, numLevelNodes, childCoords);
	}
	return levelOffsets;
}

void CompressedShadow::constructLastLevels(const MinMaxHierarchy& minMax, size_t levelOffset, size_t numNodes,
		const vector<ivec3>& childCoords) {
	constexpr uint level = 2;

	for (size_t nodeNr = 0; nodeNr < numNodes; ++nodeNr) {
		const size_t nodeOffset = levelOffset + nodeNr * LEAF_SIZE;

		const auto res = cs::createChildmask1x1x8(minMax, childCoords[nodeNr]);
		m_dag[nodeOffset] = res.first;

		size_t numChildren = res.second.size();
		for (uint childNr = 0; childNr < numChildren; ++childNr) {
			uint64 leafmask  = res.second[childNr];
			const uint index = childNr * 2 + 1;

			m_dag[nodeOffset + index]     = leafmask;
			m_dag[nodeOffset + index + 1] = leafmask >> 32;
		}
	}
}

/**
 * Given a vector of offsets to the beginning of a level and a level nr, this returns the size of the level.
 * The size of the DAG is needed for the special case level 0.
 */
inline size_t getLevelSize(const vector<uint>& dag, const vector<uint>& levelOffsets, uint level) {
	if (level == 0) {
		return (dag.size() - levelOffsets[0]);
	} else {
		return (levelOffsets[level - 1] - levelOffsets[level]);
	}
}

/**
 * Given a vector of offsets to the beginning of levels and a specific level, this tests if the level exists,
 * i.e. is valid. Levels can stop to exist if the scene is very simple and the lower levels aren't needed.
 */
inline bool levelExists(const vector<uint>& dag, const vector<uint>& levelOffsets, uint level) {
	if (level == 0)
		return dag.size() > levelOffsets[0];
	else
		return levelOffsets[level - 1] > levelOffsets[level];
}

void CompressedShadow::mergeCommonSubtrees(const vector<uint>& levelOffsets) {
	vector<uint> nodesPerLevel(m_numLevels - 2, 0);

	uint startLevel = getMinLevel(m_numLevels);

	/* Merge common subtrees bottom up (but don't merge the root node...) */
	for (uint level = startLevel; level < m_numLevels - 2; ++level) {
		if (!levelExists(m_dag, levelOffsets, level))
			continue;

		const size_t levelSize = getLevelSize(m_dag, levelOffsets, level);
		vector<uint> tempLevel(levelSize, 0);

		const size_t levelOffset = levelOffsets[level];
		const size_t nextLevelOffset = levelOffset + levelSize;

		const uint nodeSize = getNodeSize(m_numLevels, level);
		auto mapping = mergeLevel(m_dag.begin() + levelOffset, m_dag.begin() + nextLevelOffset,
				tempLevel.begin(), nodeSize, &nodesPerLevel[level]);

		std::copy(tempLevel.begin(), tempLevel.end(), m_dag.begin() + levelOffset);

		updateParentPointers(levelOffsets, mapping, level + 1);
	}

	removeUnusedNodes(levelOffsets, nodesPerLevel);
}

void CompressedShadow::updateParentPointers(const vector<uint>& levelOffsets, unordered_map<uint, uint>& mapping,
		uint parentLevel) {
	const size_t childLevelOffset  = levelOffsets[parentLevel - 1];

	const size_t parentLevelOffset = levelOffsets[parentLevel];
	const size_t parentLevelSize   = getLevelSize(m_dag, levelOffsets, parentLevel);

	for (size_t nodeOffset = parentLevelOffset; nodeOffset < parentLevelOffset + parentLevelSize; nodeOffset += NODE_SIZE) {
		/* Update child pointers which are not null */
		for (uint child = 1; child < NODE_SIZE; ++child) {
			const uint oldOffset = m_dag[nodeOffset + child];

			if (oldOffset != 0)
				m_dag[nodeOffset + child] = childLevelOffset + mapping[oldOffset - childLevelOffset];
		}
	}
}

/* Insert val at the end of the vector and maybe applies the offset */
void insertWithCorrectOffset(vector<uint>& newDag, uint val, uint offset, size_t i) {
	/* Correct offset if val is not the childmask and not 0 */
	if ((i % NODE_SIZE != 0) && val != 0) {
		assert((long)val - (long)offset > 0);
		val -= offset;
	}
	newDag.push_back(val);
}

void CompressedShadow::removeUnusedNodes(const vector<uint>& levelOffsets, const vector<uint>& numNodesPerLevel) {
	vector<uint> newDag;

	// Insert root node in new dag unconditionally
	newDag.insert(newDag.end(), m_dag.begin(), m_dag.begin() + NODE_SIZE);

	size_t levelOffset = NODE_SIZE;
	int level = m_numLevels - 3;
	int startLevel = getMinLevel(m_numLevels);

	uint offsetCorrection = 0;

	while(level >= startLevel) {
		const uint nodeSize = getNodeSize(m_numLevels, level);
		const size_t levelSize = getLevelSize(m_dag, levelOffsets, level);
		const size_t mergedLevelSize = numNodesPerLevel[level] * nodeSize;

		offsetCorrection += levelSize - mergedLevelSize;

		if (level == startLevel || offsetCorrection == 0) {
			newDag.insert(newDag.end(), m_dag.begin() + levelOffset, m_dag.begin() + levelOffset + mergedLevelSize);
		} else {
			/* Copy nodes from m_dag to newDag and correct offsets */
			for (auto i = levelOffset; i < levelOffset + mergedLevelSize; ++i) {
				insertWithCorrectOffset(newDag, m_dag[i], offsetCorrection, i);
			}
		}

		levelOffset += levelSize;
		--level;
	}

	m_dag.swap(newDag);
}

/**
 * Copies a node from the old dag to the specified new dag. Takes different nodes sizes into account.
 *
 * @param newCurrent An offset into the new dag
 * @param oldItNode An iterator from the old dag at the beginning of the node to copy.
 * @param numLevels Number of levels from the dag, i.e. m_numLevels. Needed for useLeafmasks(...)
 * @return The updated offset into the new dag
 */
template<typename ItOld>
size_t copyNodeInNewDag(vector<uint>& newDag, uint newCurrent, ItOld oldItNode, uint numChildren, uint numLevels, int level) {
	uint elems = 1 + numChildren;

	// If leafmaks are used the node size is different
	if (useLeafmasks(numLevels) && level == getMinLevel(numLevels)) {
		elems = 1 + 2 * numChildren;
	}
	newDag.insert(newDag.begin() + newCurrent, oldItNode, oldItNode + elems);
	return elems;
}

void CompressedShadow::compress() {
	vector<uint> newDag;

	int level = m_numLevels - 2;

	size_t numLevelNodes     = 1; // Number of nodes in the current level
	size_t oldDagLevelOffset = 0; // m_dag: An offset to the current level
	size_t oldDagLastLevel   = 0; // m_dag: An offset to the last level (i.e. an offset to the parent nodes)
	size_t newDagOffset      = 0; // newDag: Current offset in the dag for construction
	size_t newDagLastLevel   = 0; // newDag: Save an offset to the last level

	const int minLevel = getMinLevel(m_numLevels);

	while(level >= minLevel && numLevelNodes > 0) {
		const size_t newDagLevel = newDagOffset; // the beginning of the current level in the new DAG

		// Maps old offsets from m_dag to new offsets in newDag
		unordered_map<size_t, size_t> oldToNewOffset;

		// Set of children indices, used to get the number of (unique) new nodes
		unordered_set<uint> childrenNodesIndices;

		for (size_t nodeNr = 0; nodeNr < numLevelNodes; ++nodeNr) {
			const uint nodeSize     = getNodeSize(m_numLevels, level);
			const size_t nodeOffset = oldDagLevelOffset + nodeNr * nodeSize;
			const uint mask         = m_dag[nodeOffset];
			const uint numChildren  = cs::getNumChildren(mask);

			oldToNewOffset[nodeOffset] = newDagOffset;

			auto nodeIt = m_dag.begin() + nodeOffset;
			newDagOffset += copyNodeInNewDag(newDag, newDagOffset, nodeIt, numChildren, m_numLevels, level);

			childrenNodesIndices.insert(nodeIt + 1, nodeIt + numChildren + 1);
		}

		if (oldDagLastLevel != oldDagLevelOffset) {
			// Parent(s) exist, so update the parent(s) offsets
			size_t currentNewDagPos = newDagLastLevel;

			for (size_t currentOldPos = oldDagLastLevel; currentOldPos < oldDagLevelOffset; currentOldPos += NODE_SIZE) {
				const uint mask = m_dag[currentOldPos];
				const uint numChildren = cs::getNumChildren(mask);

				for (size_t childNr = 0; childNr < numChildren; ++childNr) {
					/* Update offsets of all children */
					size_t oldOffset = m_dag[currentOldPos + childNr + 1];
					if (oldOffset != 0) {
						newDag[currentNewDagPos + childNr + 1] = oldToNewOffset[oldOffset];
					}
				}

				currentNewDagPos += numChildren + 1;
			}
		}

		oldDagLastLevel    = oldDagLevelOffset;
		oldDagLevelOffset += numLevelNodes * NODE_SIZE;
		newDagLastLevel    = newDagLevel;
		numLevelNodes      = childrenNodesIndices.size();

		level--;
	}

	m_dag.swap(newDag);
	m_dag.shrink_to_fit();
}

uint getChildOffset(uint childmask, uint childIndex) {
	uint childBits = childIndex * 2;

	// 0xAAAA is a mask for partial visibility
	uint maskedChildMask = childmask & (0xAAAA >> (16 - childBits));

	// Count the bits set, this is the correct offset
	return POPCOUNT(maskedChildMask);
}

CompressedShadow::NodeVisibility CompressedShadow::traverse(const vec3 position, bool tryLeafmasks) {
	const ivec3 path = cs::getPathFromNDC(std::move(position), m_numLevels);

	size_t offset = 0;
	int level     = m_numLevels - 2;
	int minLevel = tryLeafmasks ? 3 : 0;

	while(level >= minLevel) {
		int lvlBit = 1 << level;
		int childIndex = ((path.x & lvlBit) ? 1 : 0) +
		                 ((path.y & lvlBit) ? 2 : 0) +
						 ((path.z & lvlBit) ? 4 : 0);

		uint childmask = m_dag[offset];

		if(isVisible(childmask, childIndex)) {
			return VISIBLE;
		} else if (isShadowed(childmask, childIndex)) {
			return SHADOW;
		} else {
			// We need the child index counting only partially visible children
			uint childOffset = getChildOffset(childmask, childIndex);

			offset = m_dag[offset + 1 + childOffset];
		}
		level -= 1;
	}

	// Read from leafmask
	if (tryLeafmasks) {
		assert(level == 2);

		int childIndex = (path.z & 0x7);
		uint childmask = m_dag[offset];

		if(isVisible(childmask, childIndex)) {
			return VISIBLE;
		} else if (isShadowed(childmask, childIndex)) {
			return SHADOW;
		}

		// We need the child index counting only partially visible children
		uint childOffset = getChildOffset(childmask, childIndex);

		uint index = offset + childOffset * 2 + 1;

		int maskIndex = (path.x & 0x7) + 8 * (path.y & 0x7);
		uint vis = 0;
		if (maskIndex < 32) {
			uint leafmask1 = m_dag[index];
			vis = leafmask1 & (1 << maskIndex);
		} else {
			uint leafmask2 = m_dag[index + 1];
			vis = leafmask2 & (1 << (maskIndex - 32));
		}
		return (vis == 0) ? SHADOW : VISIBLE;
	}


	return PARTIAL;
}
