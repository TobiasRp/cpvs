#include "CompressedShadow.h"
#include "CompressedShadowUtil.h"
#include "MinMaxHierarchy.h"
#include "ShadowMap.h"

#include <algorithm>
#include <numeric>
#include <cmath>
#include <unordered_map>
#include <future>
using namespace cs;
using namespace std;

#include <glm/ext.hpp>
#include <iostream>

CompressedShadow::CompressedShadow(const MinMaxHierarchy& minMax) {
	m_numLevels = minMax.getNumLevels();
	assert(m_numLevels > 3);

	auto levels = constructSvo(minMax);
	mergeCommonSubtrees(levels);

	initShaderAndKernels();
}

void CompressedShadow::initShaderAndKernels() {
	try {
		m_traverseCS.addShaderFromFile(GL_COMPUTE_SHADER, "../shader/traverse.cs");
		m_traverseCS.link();
	} catch(ShaderException& exc) {
		cout << exc.where() << " - " << exc.what() << endl;
		std::terminate();
	}

	m_traverseCS.addUniform("lightViewProj");
	m_traverseCS.addUniform("width");
	m_traverseCS.addUniform("height");
}

void CompressedShadow::copyToGPU() {
	m_deviceDag = make_unique<SSBO>(m_dag, GL_STATIC_READ);
}

unique_ptr<CompressedShadow> CompressedShadow::create(const MinMaxHierarchy& minMax) {
	auto cs = unique_ptr<CompressedShadow>(new CompressedShadow(minMax));

//	cs->compress();
	cs->copyToGPU();

	return cs;
}

unique_ptr<CompressedShadow> CompressedShadow::create(const ShadowMap& shadowMap) {
	MinMaxHierarchy minMax(shadowMap.createImageF());
	return create(minMax);
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

vector<uint> CompressedShadow::constructSvo(const MinMaxHierarchy& minMax) {
	const ivec3 rootOffset(0, 0, 0);
	uint64 rootmask  = cs::createChildmask(minMax, m_numLevels - 2, rootOffset);
	uint numChildren = cs::getNumChildren(rootmask);
 
	// Resize so root and children fit in
	m_dag.resize(NODE_SIZE + numChildren * NODE_SIZE);
	m_dag[0] = rootmask;

	vector<ivec3> childCoords = cs::getChildCoordinates(rootmask, rootOffset);
	setChildrenOffsets(m_dag, 0, NODE_SIZE, numChildren);

	size_t levelOffset   = NODE_SIZE;   // Offset to the beginning of the current level
	size_t numLevelNodes = numChildren; // Number of children in the current level

	/* Save level offsets so we can later traverse bottom up efficiently */
	vector<uint> levelOffsets(m_numLevels - 1, 0);
	levelOffsets[m_numLevels - 2] = 0;

	int level = m_numLevels - 3;

	/* Create new levels from the highest to the lowest level */
	while(level >= 0 && numLevelNodes > 0) {
		levelOffsets[level] = levelOffset;

		// Offset to the beginning of the next level
		size_t nextLevelOffset = levelOffset + numLevelNodes * NODE_SIZE;

		size_t newChildrenNodes  = 0; // counts the number of new children in the next level
		size_t nextLevelProgress = 0; // current index in the next level
		vector<ivec3> newChildrenCoords;

		/* First calculate the number of new children nodes so we can resize the dag.
		 * Thereby set and save all masks so we don't have to calculate them twice */
		for (size_t nodeNr = 0; nodeNr < numLevelNodes; ++nodeNr) {
			size_t nodeOffset = levelOffset + nodeNr * NODE_SIZE;
			uint64 nodemask   = cs::createChildmask(minMax, level, childCoords[nodeNr]);
			numChildren       = cs::getNumChildren(nodemask);

			m_dag[nodeOffset] = nodemask;
			newChildrenNodes += numChildren;
		}

		if (level != 0)
			m_dag.resize(m_dag.size() + newChildrenNodes * NODE_SIZE, 0);

		for (size_t nodeNr = 0; nodeNr < numLevelNodes; ++nodeNr) {
			size_t nodeOffset = levelOffset + nodeNr * NODE_SIZE;
			uint64 nodemask   = m_dag[nodeOffset];
			numChildren       = cs::getNumChildren(nodemask);

			if (numChildren > 0) {
				assert(level != 0);

				auto coords = cs::getChildCoordinates(nodemask, childCoords[nodeNr]);
				size_t childOffset = nextLevelOffset + nextLevelProgress;

				setChildrenOffsets(m_dag, nodeOffset, childOffset, numChildren);
				newChildrenCoords.insert(newChildrenCoords.end(), coords.begin(), coords.end());

				nextLevelProgress += numChildren * NODE_SIZE;
			}
		}

		levelOffset += numLevelNodes * NODE_SIZE;
		
		numLevelNodes = newChildrenNodes;
		childCoords.swap(newChildrenCoords);

		level--;
	}
	return levelOffsets;
}

/**
 * Given a vector of offsets to the beginning of a level and a level nr, this returns the size of the level.
 * The size of the DAG is needed for the special case level 0.
 */
size_t getLevelSize(const vector<uint>& dag, const vector<uint>& levelOffsets, uint level) {
	if (level == 0) {
		return (dag.size() - levelOffsets[0]);
	} else {
		return (levelOffsets[level - 1] - levelOffsets[level]);
	}
}

void CompressedShadow::mergeCommonSubtrees(const vector<uint>& levelOffsets) {
//	for_each(m_dag.begin(), m_dag.end(), [](auto val) { cout << val << ", "; });
//	cout << endl;
//	cout << endl;

	vector<uint> nodesPerLevel(m_numLevels - 2);

	/* Merge common subtrees bottom up (but don't merge the root node...) */
	for (uint level = 0; level < m_numLevels - 2; ++level) {
		const size_t levelSize = getLevelSize(m_dag, levelOffsets, level);
		vector<uint> tempLevel(levelSize, 0);

		const size_t levelOffset = levelOffsets[level];
		const size_t nextLevelOffset = levelOffset + levelSize;

		auto mapping = mergeLevel(m_dag.begin() + levelOffset, m_dag.begin() + nextLevelOffset,
				tempLevel.begin(), &nodesPerLevel[level]);

		std::copy(tempLevel.begin(), tempLevel.end(), m_dag.begin() + levelOffset);

		updateParentPointers(levelOffsets, mapping, level + 1);
	}

//	for_each(m_dag.begin(), m_dag.end(), [](auto val) { cout << val << ", "; });
//	cout << endl;
//	cout << endl;

	removeUnusedNodes(levelOffsets, nodesPerLevel);

//	for_each(m_dag.begin(), m_dag.end(), [](auto val) { cout << val << ", "; });
//	cout << endl;
//	cout << endl;
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

	uint offsetCorrection = 0;

	while(level >= 0) {
		const size_t levelSize = getLevelSize(m_dag, levelOffsets, level);
		const size_t mergedLevelSize = numNodesPerLevel[level] * NODE_SIZE;

		offsetCorrection += levelSize - mergedLevelSize;

		if (level == 0 || offsetCorrection == 0) {
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

void CompressedShadow::compress() {
	vector<uint> newDag(NODE_SIZE);

	int level = m_numLevels - 2;

	size_t numLevelNodes     = 1; // Number of nodes in the current level
	size_t oldDagLevelOffset = 0; // m_dag: An offset to the current level
	size_t oldDagLastLevel   = 0; // m_dag: An offset to the last level (i.e. an offset to the parent nodes)
	size_t newDagOffset      = 0; // newDag: Current offset in the dag for construction
	size_t newDagLastLevel   = 0; // newDag: Save an offset to the last level

	while(level >= 0 && numLevelNodes > 0) {
		const size_t newDagLevel = newDagOffset; // the beginning of the current level in the new DAG
		size_t newChildrenNodes  = 0;

		// Maps old offsets from m_dag to new offsets in newDag
		unordered_map<size_t, size_t> oldToNewOffset;

		for (size_t nodeNr = 0; nodeNr < numLevelNodes; ++nodeNr) {
			const size_t nodeOffset = oldDagLevelOffset + nodeNr * NODE_SIZE;
			const uint mask         = m_dag[nodeOffset];
			const uint numChildren  = cs::getNumChildren(mask);

			auto nodeIt = m_dag.begin() + nodeOffset;
			newDag.insert(newDag.begin() + newDagOffset, nodeIt, nodeIt + numChildren + 1);

			oldToNewOffset[nodeOffset] = newDagOffset;
			newDagOffset += numChildren + 1;

			newChildrenNodes += getNumberOfUniqueElements(nodeIt + 1, nodeIt + numChildren + 1);
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
		numLevelNodes      = newChildrenNodes;

		level--;
	}

	m_dag.swap(newDag);
	m_dag.shrink_to_fit();

	for_each(m_dag.begin(), m_dag.end(), [](auto val) { cout << val << ", "; });
	cout << endl;
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

void CompressedShadow::compute(const Texture2D* positionsWS, const mat4& lightViewProj,
		Texture2D* visibilities) {

	GL_CHECK_ERROR("traverse - begin");
	m_traverseCS.bind();

	// Bind WS positions
	positionsWS->bindImageAt(0, GL_READ_ONLY);

	// Bind image for results
	visibilities->bindImageAt(1, GL_WRITE_ONLY);

	// Bind dag
	m_deviceDag->bindAt(2);

	glUniformMatrix4fv(m_traverseCS["lightViewProj"], 1, GL_FALSE, glm::value_ptr(lightViewProj));

	// Set width and height
	const GLuint width = positionsWS->getWidth();
	const GLuint height = positionsWS->getHeight();
	glUniform1ui(m_traverseCS["width"], width);
	glUniform1ui(m_traverseCS["height"], height);

	// Now calculate work group size and dispatch!
	const GLuint localSize = 32;
	const GLuint numGroupsX = ceil(width / static_cast<float>(localSize));
	const GLuint numGroupsY = ceil(height / static_cast<float>(localSize));
	glDispatchCompute(numGroupsX, numGroupsY, 1);

	m_traverseCS.release();
	GL_CHECK_ERROR("traverse - end");
}
