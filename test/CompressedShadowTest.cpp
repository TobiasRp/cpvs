#include "CompressedShadow.h"
#include "Image.h"
#include "MinMaxHierarchy.h"
#include "gtest/gtest.h"

class CompressedShadowTest : public ::testing::Test {
protected:
	CompressedShadowTest() 
		: img(8, 8, 1)
	{
		img.setAll(vector<float>{ 
				0, 0, 0, 0, 0.1, 0.2, 0.3, 0.4,
				0, 0, 0, 0, 0.1, 0.2, 0.3, 0.4,
				0, 0, 0, 0, 0.1, 0.2, 0.2, 0.4,
				0, 0, 0, 0, 0.1, 0.2, 0.2, 0.4,
				1, 1, 0, 0, 0.1, 0.0, 0.2, 0.4,
				1, 1, 0, 0, 0.1, 0.0, 0.2, 0.4,
				1, 1, 0, 0, 0.1, 0.0, 0.2, 0.4,
				1, 1, 0, 0, 0.1, 0.0, 0.2, 1.0});
	}

	virtual ~CompressedShadowTest() {
	}

	virtual void SetUp() {
	}

	virtual void TearDown() {
	}
protected:
	ImageF img;
};

TEST_F(CompressedShadowTest, create) {
	MinMaxHierarchy mm(img);
	auto csPtr = CompressedShadow::create(mm);
	
	auto vis = csPtr->traverse(vec3(-1, 1, -1));
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(-1, -1, 0));
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	vis = csPtr->traverse(vec3(1, 1, 0));
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(0.5, 0.5, 0.5));
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(1, 1, -0.75));
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	vis = csPtr->traverse(vec3(1, 1, 0.5));
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(1, -1, 0.99));
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	vis = csPtr->traverse(vec3(0.7, -1, 0.99));
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(0.4, -1, -0.8));
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(0.1, -1.0, -0.9));
	ASSERT_EQ(CompressedShadow::SHADOW, vis);
}

