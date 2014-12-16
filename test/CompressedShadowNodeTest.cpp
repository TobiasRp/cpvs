#include "CompressedShadowNode.h"
#include "gtest/gtest.h"
#include <iostream>
using namespace std;

using namespace cs;

class CompressedShadowNodeTest : public ::testing::Test {
protected:
	CompressedShadowNodeTest() 
	{
	}

	virtual ~CompressedShadowNodeTest() {
	}

	virtual void SetUp() {
	}

	virtual void TearDown() {
	}
protected:
};

TEST_F(CompressedShadowNodeTest, testIsPartial) {
	AABB bb(ivec3(0, 0, 0), ivec3(10, 10, 10));
	Node n1(bb);

	n1.childmask = 0xAAAA;
	ASSERT_TRUE(n1.isPartial(Node::FRONT_LOWER_LEFT));
	ASSERT_FALSE(n1.isShadowed(Node::FRONT_LOWER_LEFT));
	ASSERT_FALSE(n1.isVisible(Node::FRONT_LOWER_LEFT));

	ASSERT_TRUE(n1.isPartial(Node::BACK_LOWER_RIGHT));

	n1.childmask = 0x0011;
	ASSERT_FALSE(n1.isPartial(Node::FRONT_LOWER_LEFT));
}

TEST_F(CompressedShadowNodeTest, testVisible) {
	AABB bb(ivec3(0, 0, 0), ivec3(10, 10, 10));
	Node n1(bb);

	n1.childmask = 0x1101;
	ASSERT_TRUE(n1.isVisible(Node::FRONT_LOWER_LEFT));
	ASSERT_FALSE(n1.isShadowed(Node::FRONT_LOWER_LEFT));

	ASSERT_TRUE(n1.isVisible(Node::FRONT_UPPER_RIGHT));
}

TEST_F(CompressedShadowNodeTest, testCreatingChildren) {
	ImageF img(4, 4, 1);
	img.setAll(std::vector<float>{
				0.0, 0.0, 1, 1,
				0.0, 0.0, 1, 1,
				0.75, 0.75, 0.25, 0.25,
				0.6, 0.8, 0.1, 0.3
			});

	MinMaxHierarchy mm(img);

	AABB root(ivec3(0, 0, 0), ivec3(4, 4, 4));
	auto children = root.findChildren();

	Node rootNode(std::move(root));

	rootNode.calcChildmask(mm, children, mm.getNumLevels() - 2, 0, 0);

	// Test the childmask for correctness
	ASSERT_TRUE(rootNode.isVisible(Node::FRONT_LOWER_LEFT));
	ASSERT_TRUE(rootNode.isPartial(Node::BACK_LOWER_LEFT));

	ASSERT_TRUE(rootNode.isPartial(Node::FRONT_LOWER_RIGHT));
	ASSERT_TRUE(rootNode.isShadowed(Node::BACK_LOWER_RIGHT));

	ASSERT_TRUE(rootNode.isShadowed(Node::FRONT_UPPER_LEFT));
	ASSERT_TRUE(rootNode.isShadowed(Node::BACK_UPPER_LEFT));

	ASSERT_TRUE(rootNode.isVisible(Node::FRONT_UPPER_RIGHT));
	ASSERT_TRUE(rootNode.isVisible(Node::BACK_UPPER_RIGHT));

	rootNode.addNewChildren(children);

	// Ensure that children with partial visibility have a pointer
	ASSERT_NE(nullptr, rootNode.children[(size_t)Node::BACK_LOWER_LEFT]);
	ASSERT_NE(nullptr, rootNode.children[(size_t)Node::FRONT_LOWER_RIGHT]);
}
