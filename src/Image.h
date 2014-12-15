#ifndef IMAGE_H
#define IMAGE_H

#include "cpvs.h"

/**
 * 2-dimensional image on the host.
 */
template<typename T>
class Image {
public:
	Image(size_t width, size_t height, size_t numChannels)
		: m_width(width), m_height(height), m_numChannels(numChannels) {
		m_values = vector<T>(width * height * numChannels);
	}

	~Image() = default;

	/**
	 * Set image from pointer to values which must be at least width * height * numChannels.
	 */
	void setAll(const T* ptr) {
		const unsigned size = m_width * m_height * m_numChannels;
		m_values.assign(ptr, ptr + size);
	}

	void setAll(const vector<T>& vec) {
		m_values.assign(vec.begin(), vec.end());
	}

	void setAll(typename vector<T>::iterator begin, typename vector<T>::iterator end) {
		m_values.assign(begin, end);
	}

	T get(size_t x, size_t y, size_t channel) const {
		return m_values[(y * m_width + x) * m_numChannels + channel];
	}
	void set(size_t x, size_t y, size_t channel, T val) {
		m_values[(y * m_width + x) * m_numChannels + channel] = val;
	}

	const T* data() const {
		return m_values.data();
	}

	T* data() {
		return m_values.data();
	}

	inline size_t getNumChannels() const {
		return m_numChannels;
	}

	inline size_t getWidth() const {
		return m_width;
	}

	inline size_t getHeight() const {
		return m_height;
	}

private:
	size_t m_width, m_height;
	int m_numChannels;

	vector<T> m_values;
};

using ImageF = Image<float>;

#endif
