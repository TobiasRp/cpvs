#include "MinMaxHierarchy.h"
#include "gtest/gtest.h"

class MinMaxTest : public ::testing::Test {
protected:
	MinMaxTest() 
		: img(8, 8, 1)
	{
		img.setAll(vector<float>{ 
				0.0, 0.5, 0.2, 0.7, 0.1, 0.2, 0.3, 0.4,
				0.9, 0.0, 0.0, 0.1, 1.0, 0.2, 0.3, 0.4,
				0.0, 0.5, 0.2, 0.7, 0.1, 0.2, 0.3, 0.4,
				0.0, 0.0, 1.0, 0.7, 0.1, 0.0, 0.2, 0.4,
				0.0, 0.5, 1.0, 0.7, 0.1, 0.0, 0.2, 0.4,
				0.0, 0.5, 0.2, 0.7, 0.1, 0.0, 0.2, 0.4,
				0.0, 0.0, 0.2, 0.7, 0.1, 0.0, 0.2, 0.4,
				0.0, 0.5, 1.0, 0.7, 0.1, 0.0, 0.2, 1.0});
	}

	virtual ~MinMaxTest() {
	}

	virtual void SetUp() {
	}

	virtual void TearDown() {
	}
protected:
	ImageF img;
};

TEST_F(MinMaxTest, get) {
	MinMaxHierarchy mm(img);
	ASSERT_EQ(4, mm.getNumLevels());

	ASSERT_EQ(0.0f, mm.getMin(0, 0, 0));
	ASSERT_EQ(0.0f, mm.getMax(0, 0, 0));
	ASSERT_EQ(0.0f, mm.getMin(1, 0, 0));
	ASSERT_EQ(0.9f, mm.getMax(1, 0, 0));

	ASSERT_EQ(0.0f, mm.getMin(1, 0, 0));
	ASSERT_EQ(1.0f, mm.getMax(2, 0, 0));

	ASSERT_EQ(1.0f, mm.getMax(3, 0, 0));
	ASSERT_EQ(0.0f, mm.getMin(3, 0, 0));
}

