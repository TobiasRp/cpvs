#include "CompressedShadowUtil.h"
#include "gtest/gtest.h"

using namespace cs;

TEST(getNumChildrenTest, testDifferentValues) {
	uint mask = 2;
	ASSERT_EQ(1, getNumChildren(mask));

	mask = 10;
	ASSERT_EQ(2, getNumChildren(mask));
}

TEST(getChildOffsetsTest, testChild0) {
	uint mask = 2;
	ivec3 parentOffset(0, 0, 0);
	ASSERT_EQ(parentOffset, getChildOffsets(mask, parentOffset)[0]); 
}

TEST(getChildOffsetsTest, testChild1) {
	uint mask = 8;
	ivec3 parentOffset(0, 0, 0);
	ASSERT_EQ(ivec3(2, 0, 0), getChildOffsets(mask, parentOffset)[0]); 
}

TEST(getChildOffsetsTest, testChild2) {
	uint mask = 0x0020;
	ivec3 parentOffset(0, 0, 0);
	ASSERT_EQ(ivec3(0, 2, 0), getChildOffsets(mask, parentOffset)[0]); 
}

TEST(getChildOffsetsTest, testChild3) {
	uint mask = 0x0080;
	ivec3 parentOffset(0, 0, 0);
	ASSERT_EQ(ivec3(2, 2, 0), getChildOffsets(mask, parentOffset)[0]); 
}

TEST(getChildOffsetsTest, testChild4) {
	uint mask = 0x0200;
	ivec3 parentOffset(0, 0, 0);
	ASSERT_EQ(ivec3(0, 0, 2), getChildOffsets(mask, parentOffset)[0]); 
}


TEST(getChildOffsetsTest, testChild7) {
	uint mask = 0x8000;
	ivec3 parentOffset(0, 0, 0);
	ASSERT_EQ(ivec3(2, 2, 2), getChildOffsets(mask, parentOffset)[0]); 
}
