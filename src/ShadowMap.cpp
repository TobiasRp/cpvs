#include "ShadowMap.h"

ShadowMap::ShadowMap(shared_ptr<Texture2D> tex)
	: m_texture(tex)
{
	assert(isPowerOfTwo(tex->getWidth()));
	assert(tex->getWidth() == tex->getHeight());

	assert(tex->getFormat() == GL_DEPTH_COMPONENT);
}

ImageF ShadowMap::createImageF() const {
	m_texture->bindAt(0);

	ImageF img(m_texture->getWidth(), m_texture->getHeight(), 1);
	glGetTexImage(GL_TEXTURE_2D, 0, m_texture->getFormat(), m_texture->getType(), img.data());

	return img;
}
