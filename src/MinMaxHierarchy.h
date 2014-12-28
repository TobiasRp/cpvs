#ifndef MIN_MAX_HIERARCHY_H
#define MIN_MAX_HIERARCHY_H

#include "cpvs.h"
#include "Image.h"
#include "Texture.h"

/**
 * A min-max hierarchy can be created from an Image (with 1 channel, e.g. depth values)
 * and will contain in every level (except level 0, the original image) a min-max value.
 */
class MinMaxHierarchy {
private:
	enum Channel {
		MIN_CH = 0,
		MAX_CH = 1
	};
public:
	/**
	 * Creates a min-max hierarchy for the given Image.
	 * @param orig Image where width equals height and are both a power of two.
	 */
	MinMaxHierarchy(const ImageF& orig);

	~MinMaxHierarchy() = default;

	/**
	 * Returns the minimum at (x, y) of the given level.
	 * @note For level 0 min == max
	 */
	inline float getMin(size_t level, size_t x, size_t y) const {
		if (level == 0) {
			assert(checkBounds(m_root, x, y));
			return m_root.get(x, y, 0);
		}
		assert(checkBounds(m_levels[level - 1], x, y));
		return m_levels[level - 1].get(x, y, MIN_CH);
	}

	/**
	 * Returns the maximum at (x, y) of the given level.
	 * @note For level 0 min == max
	 */
	inline float getMax(size_t level, size_t x, size_t y) const {
		if (level == 0) {
			assert(checkBounds(m_root, x, y));
			return m_root.get(x, y, 0);
		}
	
		assert(checkBounds(m_levels[level - 1], x, y));
		return m_levels[level - 1].get(x, y, MAX_CH);
	}

	/**
	 * Returns the number of levels the hierarchy has (including the original image)
	 */
	int getNumLevels() const {
		return m_levels.size() + 1;
	}

	/**
	 * Returns a level of the hierarchy.
	 */
	inline const ImageF* getLevel(size_t level) const {
		if (level == 0)
			return &m_root;
		else
			return &m_levels[level-1];
	}

private:
	/**
	 * Constructs a new level for the given one (which can't be level 0!)
	 */
	ImageF constructLevel(const ImageF& in) const;

	/**
	 * The first level 0 only contains 1 channel, therefore it needs to be processed differently.
	 */
	ImageF constructLevelFromRoot(const ImageF& in) const;

	inline bool checkBounds(const ImageF& img, size_t x, size_t y) const {
		return x < img.getWidth() && y < img.getHeight();
	}

private:
	const ImageF m_root;
	vector<ImageF> m_levels;
};

#endif
