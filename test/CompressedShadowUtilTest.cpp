#include "CompressedShadowUtil.h"
#include "gtest/gtest.h"

using namespace cs;

TEST(getNumChildrenTest, testDifferentValues) {
	uint mask = 2;
	ASSERT_EQ(1, getNumChildren(mask));

	mask = 10;
	ASSERT_EQ(2, getNumChildren(mask));
}

TEST(getChildCoordinatesTest, testChild0) {
	uint mask = 2;
	ivec3 parentOffset(0, 0, 0);
	ASSERT_EQ(parentOffset, getChildCoordinates(mask, parentOffset)[0]); 
}

TEST(getChildCoordinatesTest, testChild1) {
	uint mask = 8;
	ivec3 parentOffset(0, 0, 0);
	ASSERT_EQ(ivec3(2, 0, 0), getChildCoordinates(mask, parentOffset)[0]); 
}

TEST(getChildCoordinatesTest, testChild2) {
	uint mask = 0x0020;
	ivec3 parentOffset(0, 0, 0);
	ASSERT_EQ(ivec3(0, 2, 0), getChildCoordinates(mask, parentOffset)[0]); 
}

TEST(getChildCoordinatesTest, testChild3) {
	uint mask = 0x0080;
	ivec3 parentOffset(0, 0, 0);
	ASSERT_EQ(ivec3(2, 2, 0), getChildCoordinates(mask, parentOffset)[0]); 
}

TEST(getChildCoordinatesTest, testChild4) {
	uint mask = 0x0200;
	ivec3 parentOffset(0, 0, 0);
	ASSERT_EQ(ivec3(0, 0, 2), getChildCoordinates(mask, parentOffset)[0]); 
}


TEST(getChildCoordinatesTest, testChild7) {
	uint mask = 0x8000;
	ivec3 parentOffset(0, 0, 0);
	ASSERT_EQ(ivec3(2, 2, 2), getChildCoordinates(mask, parentOffset)[0]); 
}


TEST(isEqualSubtreeTest, testEqual) {
	vector<uint> node { 0xAAAA, 10, 42, 0, 0, 1, 2, 3, 4 };

	// Compare node with itself, which should be true
	ASSERT_TRUE(isEqualSubtree(node.begin(), node.begin()));
}

TEST(isEqualSubtreeTest, testUnEqual) {
	vector<uint> node1 { 0xAAAA, 10, 42, 0, 0, 1, 2, 3, 4 };
	vector<uint> node2 { 0xAAA0, 10, 42, 0, 0, 1, 2, 3, 4 };
	vector<uint> node3 { 0xAAAA, 10, 42, 0, 0, 1, 2, 3, 0 };

	ASSERT_FALSE(isEqualSubtree(node1.begin(), node2.begin()));
	ASSERT_FALSE(isEqualSubtree(node1.begin(), node3.begin()));
}

TEST(mergeLevelTest, testSimpleLevel) {
	vector<uint> dag {
		0xAAAA, 10, 42, 0, 0, 1, 2, 3, 4,
		0xAAA0, 10, 0, 0, 0, 0, 0, 0, 0,
		0xAAAA, 10, 42, 0, 0, 1, 2, 3, 4};

	vector<uint> res(NODE_SIZE * 2);
	auto mapping = mergeLevel(dag.begin(), dag.end(), res.begin());

	ASSERT_EQ(0, mapping[0]);
	ASSERT_EQ(NODE_SIZE, mapping[NODE_SIZE]);

	ASSERT_EQ(0xAAA0, res[NODE_SIZE]);
}
