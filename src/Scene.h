#ifndef SCENE_H
#define SCENE_H

#include "RenderProperties.h"

class Scene {
public:
	virtual ~Scene() { }

	virtual void render(RenderProperties&) const noexcept = 0;
};

#endif
