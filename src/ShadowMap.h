#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include "cpvs.h"
#include "Fbo.h"
#include "Texture.h"
#include "Image.h"

class ShadowMap {
public:
	/**
	 * Creates a Shadow Map from a Texture 2D.
	 *
	 * @note The width and height of the texture must be a power of two!
	 */
	ShadowMap(shared_ptr<Texture2D> tex);
	~ShadowMap() = default;

	shared_ptr<Texture2D> getTexture() const {
		return m_texture;
	}
	
	/**
	 * Copies the shadow map to an image in the host memory.
	 */
	unique_ptr<ImageF> createImageF() const;

private:
	shared_ptr<Texture2D> m_texture;

};

#endif
