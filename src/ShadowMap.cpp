#include "ShadowMap.h"

void clampShadowMap(Texture2D* tex) {
	tex->bindAt(0);
	tex->setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);

	float ones[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, ones);
}

ShadowMap::ShadowMap(shared_ptr<Texture2D> tex)
{
	assert(isPowerOfTwo(tex->getWidth()));
	assert(tex->getWidth() == tex->getHeight());

	assert(tex->getFormat() == GL_DEPTH_COMPONENT);

	clampShadowMap(tex.get());

	m_texture = tex;
}

ImageF ShadowMap::createImageF() const {
	m_texture->bindAt(0);

	ImageF img(m_texture->getWidth(), m_texture->getHeight(), 1);
	glGetTexImage(GL_TEXTURE_2D, 0, m_texture->getFormat(), m_texture->getType(), img.data());

	return img;
}
