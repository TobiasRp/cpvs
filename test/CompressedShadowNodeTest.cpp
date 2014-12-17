#include "CompressedShadowNode.h"
#include "TestImages.h"
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

TEST_F(CompressedShadowNodeTest, testCreatingChildren) {
	ImageF img(4, 4, 1);
	img.setAll(std::vector<float>{
				0.0, 0.0, 1, 1,
				0.0, 0.0, 1, 1,
				0.75, 0.75, 0.25, 0.25,
				0.6, 0.8, 0.1, 0.3
			});

	MinMaxHierarchy mm(img);

	Node rootNode(ivec3(0, 0, 0), 2);

	rootNode.addChildren(mm, mm.getNumLevels() - 2);

	// Test the childmask for correctness
	ASSERT_TRUE(rootNode.isVisible(Node::FRONT_LOWER_LEFT));
	ASSERT_TRUE(rootNode.isPartial(Node::BACK_LOWER_LEFT));

	ASSERT_TRUE(rootNode.isPartial(Node::FRONT_LOWER_RIGHT));
	ASSERT_TRUE(rootNode.isShadowed(Node::BACK_LOWER_RIGHT));

	ASSERT_TRUE(rootNode.isShadowed(Node::FRONT_UPPER_LEFT));
	ASSERT_TRUE(rootNode.isShadowed(Node::BACK_UPPER_LEFT));

	ASSERT_TRUE(rootNode.isVisible(Node::FRONT_UPPER_RIGHT));
	ASSERT_TRUE(rootNode.isVisible(Node::BACK_UPPER_RIGHT));

	// Ensure that children with partial visibility have a pointer
	ASSERT_NE(nullptr, rootNode.children[(size_t)Node::BACK_LOWER_LEFT]);
	ASSERT_NE(nullptr, rootNode.children[(size_t)Node::FRONT_LOWER_RIGHT]);

	// ... and the other ones don't
	ASSERT_EQ(nullptr, rootNode.children[(size_t)Node::FRONT_LOWER_LEFT]);
	ASSERT_EQ(nullptr, rootNode.children[(size_t)Node::FRONT_UPPER_LEFT]);

	auto bllChild = rootNode.children[(size_t)Node::BACK_LOWER_LEFT].get();
	bllChild->addChildren(mm, mm.getNumLevels() - 3, 2);
	ASSERT_TRUE(bllChild->isVisible(Node::FRONT_LOWER_LEFT));

	//TODO: This is an edge case! Result shouldn't be partial
	ASSERT_TRUE(bllChild->isPartial(Node::BACK_LOWER_LEFT));
}

TEST_F(CompressedShadowNodeTest, bigImageTest) {
	MinMaxHierarchy minMax(createLargeImg());

}
