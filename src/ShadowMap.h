#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include "cpvs.h"
#include "Fbo.h"
#include "Texture.h"

class ShadowMap {
public:
	ShadowMap(shared_ptr<Texture2D> tex);
	~ShadowMap() = default;

	shared_ptr<Texture2D> getTexture() const {
		return m_texture;
	}

private:
	shared_ptr<Texture2D> m_texture;
};

#endif
