#include "CompressedShadow.h"
#include "Image.h"
#include "MinMaxHierarchy.h"
#include "gtest/gtest.h"

#include <glm/ext.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// contains test depths{8x8, 16x16, 32x32}
#include "TestImages.h"

class CompressedShadowTest : public ::testing::Test {
protected:
	CompressedShadowTest() 
		: img8(8, 8, 1), img16(16, 16, 1), img32(32, 32, 1)
	{
		img8.setAll(getDepths8x8());
		img16.setAll(getDepths16x16());

		img32.setAll(getDepths32x32());
	}

	virtual ~CompressedShadowTest() {
	}

	virtual void SetUp() {
		ASSERT_TRUE(glfwInit());
		auto window = glfwCreateWindow(1, 1, "unit test", nullptr, nullptr);
		glfwMakeContextCurrent(window);
		auto err = glewInit();
		ASSERT_EQ(GLEW_OK, err);
	}


	virtual void TearDown() {
		glfwTerminate();
	}
protected:
	ImageF img8;
	ImageF img16;
	ImageF img32;
};

/* Convert the range [0,1] to [-1,1] */
inline vec3 convertToNdc(const vec3 v) {
	return v * 2.0f + -1.0f;
}

TEST_F(CompressedShadowTest, testTraverse8x8) {
	constexpr float step = 2.0f / 8.0f;

	MinMaxHierarchy mm(img8);
	auto csPtr = CompressedShadow::create(mm);
	
	auto vis = csPtr->traverse(vec3(-1, 1, -1), false);
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(-1, -1, 0), false);
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	vis = csPtr->traverse(vec3(1, 1, 0), false);
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(0, 0, 0), false);
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(0.5, 0.5, 0.5), false);
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(1, 1, -0.75), false);
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	vis = csPtr->traverse(vec3(1, 1, 0.5), false);
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(1, -1, 0.99), false);
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	vis = csPtr->traverse(vec3(0.7, -1, 0.99), false);
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(.45, -1, -0.9), false);
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(0.1, -1.0, -0.9), false);
	ASSERT_EQ(CompressedShadow::SHADOW, vis);
}

TEST_F(CompressedShadowTest, testTraverse16x16) {
	constexpr float step = 2.0f / 16.0f;

	MinMaxHierarchy mm(img16);

	auto csPtr = CompressedShadow::create(mm);
	
	auto vis = csPtr->traverse(vec3(1, 1, 0));
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	vis = csPtr->traverse(vec3(-1, -1, -1.0f + 3 * step ));
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	vis = csPtr->traverse(vec3(-1, -step, -1.0f + 3 * step ));
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	vis = csPtr->traverse(vec3(-1, 1, -1));
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(0, 0, -.9));
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	vis = csPtr->traverse(vec3(0 + step, 0, .9));
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(0 + 3 * step, 0, -.9));
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);
}

TEST_F(CompressedShadowTest, testTraverse32x32) {
	constexpr float step = 2.0f / 32.0f;

	MinMaxHierarchy mm(img32);

	auto csPtr = CompressedShadow::create(mm);
	
	auto vis = csPtr->traverse(vec3(1, 1, 0));
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);
}

TEST_F(CompressedShadowTest, testCombiningShadows) {
	vector<unique_ptr<CompressedShadow>> shadows;
	shadows.reserve(8);

	// Fill (0, 0, 0) with 1's
	{
		ImageF ones(8, 8, 1);
		ones.setAll(getOnes8x8());
		MinMaxHierarchy mmOne(ones);
		shadows.push_back(CompressedShadow::create(mmOne));
	}

	// Fill (1, 0, 0) with 0's
	{
		ImageF zeroes(8, 8, 1);
		zeroes.setAll(getZeroes8x8());
		MinMaxHierarchy mm(zeroes);
		shadows.push_back(CompressedShadow::create(mm));
	}

	// And the rest with the default test values, see TestImages.cpp
	for (uint i = 2; i < 8; ++i) {
		MinMaxHierarchy mm(img8);
		shadows.push_back(CompressedShadow::create(mm));
	}

	auto combinedCS = CompressedShadow::combine(shadows);

	// Test (0, 0, 0) - should be visible everywhere
	auto vis = combinedCS->traverse(vec3(-1, -1, -0.1), false);
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	// Test (1, 0, 0) - should be shadowed everywhere
	vis = combinedCS->traverse(vec3(0.5, -0.5, -0.1), false);
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	// Test (0, 0, 1) - using the default 8x8 test values
	vis = combinedCS->traverse(vec3(-0.05, -1, 0.3), false);
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	vis = combinedCS->traverse(vec3(-0.05, -0.05, 1.0), false);
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = combinedCS->traverse(vec3(-1.0, -0.05, 0.9), false);
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = combinedCS->traverse(vec3(-1.0, -0.95, 0.9), false);
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

}
