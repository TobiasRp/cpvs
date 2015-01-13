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
	 * @note The width must be equal to the height and both must be a power of two!
	 */
	ShadowMap(shared_ptr<Texture2D> tex);

	/**
	 * Creates a Shadow Map from multiple 2-dimensional textures.
	 */
	ShadowMap(const vector<shared_ptr<Texture2D>>& textures)
		: m_textures(textures) { }

	ShadowMap() = default;
	~ShadowMap() = default;

	shared_ptr<Texture2D> getTexture(uint index) const {
		return m_textures[index];
	}

	size_t getNumTextures() const {
		return m_textures.size();
	}

	void add(shared_ptr<Texture2D> tex);
	
	/**
	 * Copies the specified texture of the shadow map to an image in the host memory.
	 */
	ImageF createImageF(uint index) const;

private:
	vector<shared_ptr<Texture2D>> m_textures;
};

#endif
