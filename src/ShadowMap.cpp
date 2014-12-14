#include "ShadowMap.h"

ShadowMap::ShadowMap(shared_ptr<Texture2D> tex)
	: m_texture(tex)
{
	assert(isPowerOfTwo(tex->getWidth()));
	assert(isPowerOfTwo(tex->getHeight()));

	assert(tex->getInternalFormat() == GL_R32F);
}

unique_ptr<ImageF> ShadowMap::createImageF() const {
	m_texture->bindAt(0);

	auto img = std::make_unique<ImageF>(m_texture->getWidth(), m_texture->getHeight(), 1);
	glGetTexImage(GL_TEXTURE_2D, 0, m_texture->getFormat(), m_texture->getType(), img->data());

	return std::move(img);
}
