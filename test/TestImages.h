#ifndef TEST_IMAGES_H
#define TEST_IMAGES_H
#include "cpvs.h"

extern vector<float> getDepths8x8();

extern vector<float> getDepths16x16();

extern vector<float> getDepths32x32();

inline vector<float> getZeroes(uint dimension) {
	return vector<float>(dimension * dimension, 0.0);
}

inline vector<float> getOnes(uint dimension) {
	return vector<float>(dimension * dimension, 1.0);
}

#endif
