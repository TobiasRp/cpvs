#include "MinMaxHierarchy.h"
#include <cmath>
#include <thread>
#include <functional>

// Under this threshold stop processing in parallel
#define PARALLEL_THRESHOLD 256

MinMaxHierarchy::MinMaxHierarchy(const ImageF& orig)
	: m_root(orig)
{
	assert(orig.getWidth() == orig.getHeight());
	const int size = orig.getWidth();
	assert(isPowerOfTwo(size));

	// num of levels (without root)
	const int numLevels = std::ceil(log2(size));
	m_levels.reserve(numLevels);

	ImageF level1 = constructLevelFromRoot(orig);
	m_levels.push_back(std::move(level1));

	for (unsigned i = 0; i < numLevels - 1; ++i) {
		ImageF newLevel = constructLevel(m_levels[i]);
		m_levels.push_back(std::move(newLevel));
	}
}

inline float find(float a, float b, float c, float d, std::function<float(float, float)> pred) {
	float abRes = pred(a, b);
	float cdRes = pred(c, d);
	return pred(abRes, cdRes);
}

void setRow(const ImageF& in, ImageF& out, int row, int channel, int outChannel, std::function<float(float, float)> pred) {
	for (int x = 0; x < in.getWidth(); x += 2) {
		float val = find(in.get(x, row, channel), in.get(x + 1, row, channel),
				in.get(x, row + 1, channel), in.get(x + 1, row + 1, channel), pred);

		out.set(x / 2, row / 2, outChannel, val);
	}
}

void constructRange(const ImageF& in, ImageF& out, int begin, int end, int minInChannel, int maxInChannel) {
	for (int y = begin; y < end; y += 2) {
		setRow(in, out, y, minInChannel, 0,std::min<float>);
		setRow(in, out, y, maxInChannel, 1, std::max<float>);
	}
}

ImageF MinMaxHierarchy::constructLevel(const ImageF& in) const {
	const int inSize = in.getWidth();
	const int newSize = inSize / 2;
	ImageF res(newSize, newSize, 2);

	if (inSize >= PARALLEL_THRESHOLD) {
		const int perThreadWork = inSize / 4;

		std::thread task1{constructRange, std::cref(in), std::ref(res), 0, perThreadWork, MIN_CH, MAX_CH};
		std::thread task2{constructRange, std::cref(in), std::ref(res), perThreadWork, 2 * perThreadWork, MIN_CH, MAX_CH};
		std::thread task3{constructRange, std::cref(in), std::ref(res), 2 * perThreadWork, 3 * perThreadWork, MIN_CH, MAX_CH};
		constructRange(in, res, 3 * perThreadWork, inSize, MIN_CH, MAX_CH);

		task1.join();
		task2.join();
		task3.join();
	} else {
		constructRange(in, res, 0, inSize, MIN_CH, MAX_CH);
	}

	return res;
}

ImageF MinMaxHierarchy::constructLevelFromRoot(const ImageF& in) const {
	const int inSize = in.getWidth();
	const int newSize = inSize / 2;
	ImageF res(newSize, newSize, 2);

	const int perThreadWork = inSize / 4;

	std::thread task1{constructRange, std::cref(in), std::ref(res), 0, perThreadWork, 0, 0};
	std::thread task2{constructRange, std::cref(in), std::ref(res), perThreadWork, 2 * perThreadWork, 0, 0};
	std::thread task3{constructRange, std::cref(in), std::ref(res), 2 * perThreadWork, 3 * perThreadWork, 0, 0};
	constructRange(in, res, 3 * perThreadWork, inSize, 0, 0);

	task1.join();
	task2.join();
	task3.join();

	return res;
}

float MinMaxHierarchy::getMin(int level, int x, int y) const {
	if (level == 0)
		return m_root.get(x, y, 0);
	return m_levels[level - 1].get(x, y, MIN_CH);
}

float MinMaxHierarchy::getMax(int level, int x, int y) const {
	if (level == 0)
		return m_root.get(x, y, 0);
	return m_levels[level - 1].get(x, y, MAX_CH);
}
