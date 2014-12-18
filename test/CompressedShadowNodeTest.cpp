#include "CompressedShadowNode.h"
#include "gtest/gtest.h"
#include <iostream>
using namespace std;

using namespace cs;

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
	}

	virtual ~CompressedShadowNodeTest() {
	}

	virtual void SetUp() {
		rootNode.addChildren(*(mm.get()), mm->getNumLevels() - 2);
	}

	virtual void TearDown() {
	}
protected:
	Node rootNode;
	unique_ptr<MinMaxHierarchy> mm;
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
