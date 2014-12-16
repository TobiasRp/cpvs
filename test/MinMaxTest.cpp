#include "MinMaxHierarchy.h"
#include "gtest/gtest.h"

#include <iostream>
using namespace std;

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

TEST_F(MinMaxTest, create4x4) {
	ImageF img4x4(4, 4, 1);
	img4x4.setAll(std::vector<float>{
				0.0, 0.0, 1, 1,
				0.0, 0.0, 1, 1,
				0.75, 0.75, 0.25, 0.25,
				0.6, 0.8, 0.1, 0.3
			});

	MinMaxHierarchy mm(img4x4);

	ASSERT_EQ(0.0, mm.getMin(1, 0, 0));
	ASSERT_EQ(0.0, mm.getMax(1, 0, 0));
	ASSERT_EQ(1.0, mm.getMax(1, 1, 0));
}
