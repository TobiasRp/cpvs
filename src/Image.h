#ifndef IMAGE_H
#define IMAGE_H

#include "cpvs.h"

/**
 * 2-dimensional image on the host.
 */
template<typename T>
class Image {
public:
	Image(int width, int height, int numChannels)
		: m_width(width), m_height(height), m_numChannels(numChannels) {
		m_values = vector<T>(width * height * numChannels);
	}

	~Image() { }

	/**
	 * Set image from pointer to values which must be at least width * height * numChannels.
	 */
	void setAll(const T* ptr) {
		const unsigned size = m_width * m_height * m_numChannels;
		m_values.assign(ptr, ptr + size);
	}

	void setAll(const vector<T> vec) {
		m_values.assign(vec.begin(), vec.end());
	}

	T get(int x, int y, int channel) const {
		return m_values[(y * m_width + x) * m_numChannels + channel];
	}

	void set(int x, int y, int channel, T val) {
		m_values[(y * m_width + x) * m_numChannels + channel] = val;
	}
	
	T* data() {
		return m_values.data();
	}

	inline int getWidth() const {
		return m_width;
	}

	inline int getHeight() const {
		return m_height;
	}

private:
	int m_width, m_height;
	int m_numChannels;

	vector<T> m_values;
};

using ImageF = Image<float>;

#endif
