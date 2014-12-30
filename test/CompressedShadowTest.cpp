#include "CompressedShadow.h"
#include "Image.h"
#include "MinMaxHierarchy.h"
#include "gtest/gtest.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

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

class CompressedShadowTest : public ::testing::Test {
protected:
	CompressedShadowTest() 
		: img8(8, 8, 1), img16(16, 16, 1)
	{
		img8.setAll(vector<float>{ 
				0, 0, 0, 0, 0.1, 0.2, 0.3, 0.4,
				0, 0, 0, 0, 0.1, 0.2, 0.3, 0.4,
				0, 0, 0, 0, 0.1, 0.2, 0.2, 0.4,
				0, 0, 0, 0, 0.1, 0.2, 0.2, 0.4,
				1, 1, 0, 0, 0.1, 0.0, 0.2, 0.4,
				1, 1, 0, 0, 0.1, 0.0, 0.2, 0.4,
				1, 1, 0, 0, 0.1, 0.0, 0.2, 0.4,
				1, 1, 0, 0, 0.1, 0.0, 0.2, 1.0});

		img16.setAll(depths16x16);
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
};

TEST_F(CompressedShadowTest, testTraverse8x8) {
	constexpr float step = 2.0f / 8.0f;

	MinMaxHierarchy mm(img8);
	auto csPtr = CompressedShadow::create(mm);
	
	auto vis = csPtr->traverse(vec3(-1, 1, -1));
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(-1, -1, 0));
	ASSERT_EQ(CompressedShadow::VISIBLE, vis);

	vis = csPtr->traverse(vec3(1, 1, 0));
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(0, 0, 0));
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

	vis = csPtr->traverse(vec3(.45, -1, -0.9));
	ASSERT_EQ(CompressedShadow::SHADOW, vis);

	vis = csPtr->traverse(vec3(0.1, -1.0, -0.9));
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
}

TEST_F(CompressedShadowTest, testCompute8) {
	MinMaxHierarchy mm(img8);
	auto csPtr = CompressedShadow::create(mm);

	// Some test positions in the range [-1, 1]^4. 
	// These values give a nice visibility pattern (which is easy to test), see below.
	ImageF positions(8, 8, 4);
	positions.setAll(vector<float>{
		// 1st row
		-1, 1, -.99, 1,  -.65, 1, -.99, 1,  -.4, 1, -.9, 1,  -.15, 1, -.99, 1,
		.1, 1,    0, 1,   .40, 1,    0, 1,  .65, 1,   0, 1,  1, 1, -.4, 1,

		// 2nd row 
		-1, .65, -.99, 1,  -.65, .65, -.99, 1,  -.4, .65, -.99, 1,  -.15, .65, -.99, 1,
		.1, .65,    0, 1,   .40, .65,    0, 1,  .65, .65,    0, 1,    1, .65,  -.4, 1,

		// 3rd ...
		-1, .4, -.9, 1,  -.65, .4, -.9, 1,  -.4, .4, -.9, 1,  -.15, .4, -.9, 1,
		.1, .4,   0, 1,   .40, .4, 0, 1,  .65, .4, -.6, 1,  1, .4, -.4, 1,

		-1, .1, -.9, 1,  -.65, .1, -.9, 1,  -.4, .1, -.9, 1,  -.15, .1, -.9, 1,
		.1, .1, 0, 1,   .40, .1, 0, 1,  .65, .1, 0, 1,  1, .1, -.4, 1,

		-1, -.15, 0, 1,  -.65, -.15, 0, 1,  -.4, -.15, 0, 1,  -.15, -.15, 0, 1,
		.1, -.15, 0, 1,   .40, -.15, 0, 1,  .65, -.15, 0, 1,  1, -.15, -.4, 1,

		-1, -.4, 0, 1,  -.65, -.4, 0, 1,  -.4, -.4, 0, 1,  -.15, -.4, 0, 1,
		.1, -.4, 0, 1,   .40, -.4, 0, 1,  .65, -.4, 0, 1,  1, -.4, -.4, 1,

		-1, -.65, 0, 1,  -.65, -.65, 0, 1,  -.4, -.65, 0, 1,  -.15, -.65, 0, 1,
		.1, -.65, 0, 1,   .40, -.65, 0, 1,  .65, -.65, 0, 1,   1, -.65, -.4, 1,

		-1, -1,  .99, 1,  -.65, -1, .99, 1,  -.4, -1, -.99, 1,  -.15, -1, -.99, 1,
		.1, -1, -.89, 1,   .40, -1, -.89, 1,  .65, -1, -.74, 1,  1, -1, 0.9999, 1});

	Texture2D positionsTex(positions);

	mat4 lightViewProj(1); //identity matrix 4x4

	Texture2D result(8, 8, GL_R8, GL_RED, GL_FLOAT);

	csPtr->compute(&positionsTex, lightViewProj, &result);
	ASSERT_EQ(GL_NO_ERROR, glGetError());

	result.bindAt(0);
	ImageF resultImg(8, 8, 1);
	glGetTexImage(GL_TEXTURE_2D, 0, result.getFormat(), result.getType(), resultImg.data());

	/* visibilites are now in resultImg and they should be:
	 * 00000001
	 * 00000001
	 * 00000001
	 * 00000001
	 * 11000001
	 * 11000001
	 * 11000001
	 * 11000001
	 */
	for(size_t y = 0; y < 8; ++y) {
		for (size_t x = 0; x < 8; ++x) {
			// Test some visibilities (which are heaviliy influenced by the test positions above!)

			int vis = resultImg.get(x, y, 0);

			// right border is visible
			if (x % 8 == 7)
				ASSERT_EQ(1, vis);
			// in the lower half the two leftmost columns should be visible
			else if (y >= 4 && (x == 0 || x == 1))
				ASSERT_EQ(1, vis);
			else
				ASSERT_EQ(0, vis);
		}
	}
}
