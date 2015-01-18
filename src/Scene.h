#ifndef SCENE_H
#define SCENE_H

#include "cpvs.h"
#include "RenderProperties.h"

struct AABB {
	vec3 min, max;

	vec3 getCenter() const {
		vec3 dist = max - min;
		return min + (dist * 0.5f);
	}
};

class Scene {
public:
	virtual ~Scene() { }

	virtual void render(RenderProperties&) const noexcept = 0;

	inline AABB getBoundingBox() const {
		return m_boundingBox;
	}

protected:
	AABB m_boundingBox;
};

#endif
