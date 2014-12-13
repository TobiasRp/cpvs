#include "ShadowMap.h"

ShadowMap::ShadowMap(shared_ptr<Texture2D> tex)
	: m_texture(tex)
{
	//glGetTexImage(GL_TEXTURE_2D, 0, format, type, *image);
}
