#include "CompressedShadow.h"
#include "CompressedShadowNode.h"
#include "MinMaxHierarchy.h"
#include "ShadowMap.h"
#include <cmath>
using namespace cs;
using namespace std;

// for testing and debugging
#include <glm/ext.hpp>
#include <iostream>

CompressedShadow::~CompressedShadow() {
	delete root;
}

CompressedShadow::CompressedShadow(const MinMaxHierarchy& minMax) {
	m_numLevels = minMax.getNumLevels();

	size_t res = getResolution(m_numLevels);
	assert(res >= 2);

	root = new Node(ivec3(0, 0, 0), 2);
	constructSvoSubtree(minMax, m_numLevels - 2, root);
}

void CompressedShadow::constructSvoSubtree(const MinMaxHierarchy& minMax, size_t level, cs::Node* top) {
	top->addChildren(minMax, level);

	if (level == 0)
		return;

	for (auto& childPtr : top->children) {
		if (childPtr != nullptr) {
			constructSvoSubtree(minMax, level - 1, childPtr.get());
		}
	}
}

unique_ptr<CompressedShadow> CompressedShadow::create(const MinMaxHierarchy& minMax) {
	return unique_ptr<CompressedShadow>(new CompressedShadow(minMax));
}

unique_ptr<CompressedShadow> CompressedShadow::create(const ShadowMap& shadowMap) {
	MinMaxHierarchy minMax(shadowMap.createImageF());
	return create(minMax);
}


CompressedShadow::NodeVisibility CompressedShadow::traverse(vec3 position) {
	ivec3 path = getPathFromNDC(position, m_numLevels);

//	cout << glm::to_string(path) << endl;
	return traverse(path);
}

CompressedShadow::NodeVisibility CompressedShadow::traverse(ivec3 path) {
	Node *node = root;
	int level = m_numLevels - 2;
	while(level >= 0) {
		int lvlBit = 1 << (level + 1);
		//cout << "lvlBit = " << hex << lvlBit << endl;
		int childIndex = ((path.x & lvlBit) ? 1 : 0) +
		                  ((path.y & lvlBit) ? 2 : 0) +
						  ((path.z & lvlBit) ? 4 : 0);

		//cout << childIndex << endl;

		if (node->isVisible((Node::NodeNumber) childIndex)) {
			return VISIBLE;
		} else if (node->isShadowed((Node::NodeNumber) childIndex)) {
			return SHADOW;
		} else {
			node = node->children[childIndex].get();
			assert(node != nullptr);
		}

		level -= 1;
	}


	return PARTIAL;
}
