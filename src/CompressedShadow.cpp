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


CompressedShadow::CompressedShadow(const MinMaxHierarchy& minMax) {
	m_numLevels = minMax.getNumLevels();

	size_t res = getResolution(m_numLevels);
	assert(res >= 2);

	auto root = make_unique<Node>(ivec3(0, 0, 0), 2);
	constructSvoSubtree(minMax, m_numLevels - 2, root.get());
}

void CompressedShadow::constructSvoSubtree(const MinMaxHierarchy& minMax, size_t level, cs::Node* top) {
	top->addChildren(minMax, level);

	if (level == 0)
		return;

	for (auto& childPtr : top->children) {
		if (childPtr != nullptr) {
			cout << "next level shit: " << level - 1<< endl;
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
	return PARTIAL;
}
