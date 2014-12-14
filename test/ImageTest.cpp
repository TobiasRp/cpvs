#include "Image.h"
#include "gtest/gtest.h"

class ImageTest : public ::testing::Test {
protected:
	ImageTest() {
	}

	virtual ~ImageTest() {
	}

	virtual void SetUp() {
	}

	virtual void TearDown() {
	}
};

TEST_F(ImageTest, getSet) {
	ImageF img(64, 64, 2);

	img.set(2, 2, 0, 42.0f);
	img.set(2, 2, 1, 21.0f);
	ASSERT_EQ(img.get(2, 2, 0), 42.0f);
	ASSERT_EQ(img.get(2, 2, 1), 21.0f);
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
