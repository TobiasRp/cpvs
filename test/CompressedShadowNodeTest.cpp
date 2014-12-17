#include "CompressedShadowNode.h"
#include "gtest/gtest.h"
#include <iostream>
using namespace std;

using namespace cs;

std::vector<float> depths16x16 {
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.4, 0.3, 0.2, 0.4, 0.4, 0.3, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.4, 0.3, 0.2, 0.4, 0.4, 0.3, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.4, 0.3, 0.2, 0.4, 0.4, 0.3, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.4, 0.3, 0.2, 0.4, 0.4, 0.3, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.4, 0.3, 0.2, 0.4, 0.4, 0.3, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.4, 0.3, 0.2, 0.4, 0.4, 0.3, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.4, 0.3, 0.2, 0.4, 0.4, 0.3, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.4, 0.3, 0.2, 0.4, 0.4, 0.3, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0
};


class CompressedShadowNodeTest : public ::testing::Test {
protected:
	CompressedShadowNodeTest() {
		ImageF img(4, 4, 1);
		img.setAll(std::vector<float>{
					0.0, 0.0, 1, 1,
					0.0, 0.0, 1, 1,
					0.75, 0.75, 0.25, 0.25,
					0.8, 0.8, 0.1, 0.3
				});

		mm = make_unique<MinMaxHierarchy>(img);

		ImageF bigImg(16, 16, 1);
		bigImg.setAll(depths16x16);
		bigMinMax = make_unique<MinMaxHierarchy>(bigImg);
	}

	virtual ~CompressedShadowNodeTest() {
	}

	virtual void SetUp() {
		rootNode.addChildren(*(mm.get()), mm->getNumLevels() - 2);

		bigRootNode.addChildren(*(bigMinMax.get()), bigMinMax->getNumLevels() - 2);
	}

	virtual void TearDown() {
	}
protected:
	Node rootNode;
	unique_ptr<MinMaxHierarchy> mm;

	unique_ptr<MinMaxHierarchy> bigMinMax;
	Node bigRootNode;
};

TEST_F(CompressedShadowNodeTest, testCreatingChildren) {
	// Test the childmask for correctness
	ASSERT_TRUE(rootNode.isVisible(Node::FRONT_LOWER_LEFT));
	ASSERT_TRUE(rootNode.isPartial(Node::BACK_LOWER_LEFT));

	ASSERT_TRUE(rootNode.isPartial(Node::FRONT_LOWER_RIGHT));
	ASSERT_TRUE(rootNode.isShadowed(Node::BACK_LOWER_RIGHT));

	ASSERT_TRUE(rootNode.isShadowed(Node::FRONT_UPPER_LEFT));
	ASSERT_TRUE(rootNode.isShadowed(Node::BACK_UPPER_LEFT));

	ASSERT_TRUE(rootNode.isVisible(Node::FRONT_UPPER_RIGHT));
	ASSERT_TRUE(rootNode.isVisible(Node::BACK_UPPER_RIGHT));
}

TEST_F(CompressedShadowNodeTest, testChildPointers) {
	// Ensure that children with partial visibility have a pointer
	ASSERT_NE(nullptr, rootNode.children[(size_t)Node::BACK_LOWER_LEFT]);
	ASSERT_NE(nullptr, rootNode.children[(size_t)Node::FRONT_LOWER_RIGHT]);

	// ... and the other ones don't
	ASSERT_EQ(nullptr, rootNode.children[(size_t)Node::FRONT_LOWER_LEFT]);
	ASSERT_EQ(nullptr, rootNode.children[(size_t)Node::FRONT_UPPER_LEFT]);
}

TEST_F(CompressedShadowNodeTest, testSubNode) {
	auto bllChild = rootNode.children[(size_t)Node::BACK_LOWER_LEFT].get();
	bllChild->addChildren(*(mm.get()), mm->getNumLevels() - 3, 2);
	ASSERT_TRUE(bllChild->isVisible(Node::FRONT_LOWER_LEFT));
	ASSERT_TRUE(bllChild->isShadowed(Node::BACK_LOWER_LEFT));
}

TEST_F(CompressedShadowNodeTest, bigHierarchyTest) {
	// In the big image all 8 children are partially visible
	for (uint i = 0; i < 8; ++i)
		ASSERT_TRUE(bigRootNode.isPartial((Node::NodeNumber)i));
}
