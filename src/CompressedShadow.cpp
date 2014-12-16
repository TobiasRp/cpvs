#include "CompressedShadow.h"
#include "CompressedShadowNode.h"
#include "MinMaxHierarchy.h"
#include "ShadowMap.h"

#include <cmath>
#include <glm/ext.hpp>
#include <iostream>
using namespace std;
using namespace cs;

CompressedShadow::CompressedShadow(const MinMaxHierarchy& minMax) {
	m_numLevels = minMax.getNumLevels();

	size_t res = getResolution(m_numLevels);
	assert(res >= 2);

	AABB lightFrustum(ivec3(0, 0, 0), ivec3(2, 2, 2));
	m_root = new Node(lightFrustum); //TODO use unique_ptr
	constructSvoSubtree(minMax, m_numLevels - 2, m_root);
}

void CompressedShadow::constructSvoSubtree(const MinMaxHierarchy& minMax, size_t level, cs::Node* top) {
	auto children = top->aabb.findChildren();
	size_t offsetX = top->aabb.minPoint.x;
	size_t offsetY = top->aabb.minPoint.y;

	top->calcChildmask(minMax, children, level, offsetX, offsetY);
	top->addNewChildren(children);

	if (level == 0)
		return;

	for (auto& childPtr : top->children) {
		if (childPtr != nullptr) {
			/* The AABB now needs to be of a bigger resolution
			 * (because it will be used on the next level), so double it's size*/
			childPtr->doubleAABB();

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
	cout << "path = " << glm::to_string(getPathFromNDC(position, m_numLevels)) << endl;
	int level = m_numLevels - 2;
	Node *ptr = m_root;
	while(level >= 0) {

		if (!ptr->isPartial((Node::NodeNumber) 1)) {
			cout << "returning at level " << level << endl;
			if (ptr->isVisible((Node::NodeNumber) 0))
				return VISIBLE;
			else
				return SHADOW;
		}
		cout << "continuing at level " << level << endl;
		ptr = ptr->children[0].get();
		level--;
	}

	return PARTIAL;
}
