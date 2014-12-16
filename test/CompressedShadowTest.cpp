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
				0, 0, 0, 0, 0.1, 0.2, 0.2, 0.4,
				0, 0, 0, 0, 0.1, 0.2, 0.2, 0.4,
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

	ASSERT_EQ(0.0, mm.getMin(3, 0, 0));

	//ASSERT_EQ(CompressedShadow::VISIBLE, csPtr->traverse(vec3(-1.0f, -1.0f, -1.0f)));
}

