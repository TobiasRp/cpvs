#include "CompressedShadow.h"
#include "CompressedShadowUtil.h"
#include "MinMaxHierarchy.h"
#include "ShadowMap.h"

#include <algorithm>
#include <numeric>
#include <cmath>
#include <unordered_map>
using namespace cs;
using namespace std;

#include <glm/ext.hpp>
#include <iostream>

constexpr uint NODE_SIZE = 9; // childmask + 8 pointers (unused pointer will be removed later)

CompressedShadow::CompressedShadow(const MinMaxHierarchy& minMax) {
	m_numLevels = minMax.getNumLevels();
	assert(m_numLevels > 3);

	constructSvo(minMax);

	compress();

	initShaderAndKernels();
	m_deviceDag = make_unique<SSBO>(m_dag, GL_STATIC_READ);
}

void CompressedShadow::initShaderAndKernels() {
	try {
		m_traverseCS.addShaderFromFile(GL_COMPUTE_SHADER, "../shader/traverse.cs");
		m_traverseCS.link();
	} catch(ShaderException& exc) {
		cout << exc.where() << " - " << exc.what() << endl;
		std::terminate();
	}

	m_traverseCS.addUniform("shadowProj");
	m_traverseCS.addUniform("width");
	m_traverseCS.addUniform("height");
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

	size_t levelOffset     = NODE_SIZE;
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
	m_dag.shrink_to_fit();
}

/** Helper function which simplifies getting the number of children */
inline uint getNumChildrenFromOffset(const vector<uint>& dag, size_t offset) {
	const uint64 nodemask = dag[offset];
	return cs::getNumChildren(nodemask);
}

void CompressedShadow::compress() {
	vector<uint> newDag(1);

	int level = m_numLevels - 2;

	size_t numLevelNodes  = 1; // Number of nodes in the current level
	size_t oldLevelOffset = 0; // m_dag: An offset to the current level
	size_t oldLastLevel   = 0; // m_dag: An offset to the last level (i.e. an offset to the parent nodes)
	size_t newDagOffset   = 0; // newDag: Current offset in the dag for construction
	size_t newLastLevel   = 0; // newDag: Save an offset to the last level

	while(level >= 0 && numLevelNodes > 0) {
		const size_t newDagLevel = newDagOffset; // the beginning of the current level in the new DAG
		size_t newChildrenNodes  = 0;

		// Maps old offsets from m_dag to new offsets in newDag
		unordered_map<size_t, size_t> oldToNewOffset;

		for (size_t nodeNr = 0; nodeNr < numLevelNodes; ++nodeNr) {
			const size_t nodeOffset = oldLevelOffset + nodeNr * NODE_SIZE;
			const uint numChildren = getNumChildrenFromOffset(m_dag, nodeOffset);

			auto nodeIt = m_dag.begin() + nodeOffset;
			newDag.insert(newDag.begin() + newDagOffset, nodeIt, nodeIt + numChildren + 1);

			oldToNewOffset[nodeOffset] = newDagOffset;

			newDagOffset     += numChildren + 1;
			newChildrenNodes += numChildren;
		}

		if (oldLastLevel != oldLevelOffset) {
			// Parent(s) exist, so update the parent(s) offsets
			size_t currentNewDagPos = newLastLevel;
			size_t currentOldPos = oldLastLevel;
			while (currentOldPos != oldLevelOffset) {
				const uint numChildren = getNumChildrenFromOffset(m_dag, currentOldPos);

				for (size_t childNr = 0; childNr < numChildren; ++childNr) {
					/* Update offsets of all children */
					size_t oldOffset = m_dag[currentOldPos + childNr + 1];
					newDag[currentNewDagPos + childNr + 1] = oldToNewOffset[oldOffset];
				}

				currentOldPos    += NODE_SIZE;
				currentNewDagPos += numChildren + 1;
			}
		}

		oldLastLevel    = oldLevelOffset;
		oldLevelOffset += numLevelNodes * NODE_SIZE;
		newLastLevel    = newDagLevel;
		numLevelNodes   = newChildrenNodes;

		level--;
	}

	m_dag.swap(newDag);
	m_dag.shrink_to_fit();
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

	glUniformMatrix4fv(m_traverseCS["shadowProj"], 1, GL_FALSE, glm::value_ptr(lightViewProj));

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
