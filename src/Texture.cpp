#include "Texture.h"

Texture1D::Texture1D(GLuint width, GLint internalFormat, GLenum format, GLenum type)
	: TextureBase(internalFormat, format, type), m_width(width)
{
	assert(width < GL_MAX_TEXTURE_SIZE);
	glBindTexture(GL_TEXTURE_1D, m_id);

	setMinMagFiltering(GL_NEAREST, GL_NEAREST);
	setWrap(GL_CLAMP_TO_BORDER);

	glTexImage1D(GL_TEXTURE_1D, 0, m_internalFormat, width, 0, format, type, nullptr);
	GL_CHECK_ERROR("Texture1D::Texture1D() - ERROR: ");
}

void Texture1D::setMinMagFiltering(GLint min, GLint max) {
	glBindTexture(GL_TEXTURE_1D, m_id);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, max);
}

void Texture1D::setWrap(GLint wrapS) {
	glBindTexture(GL_TEXTURE_1D, m_id);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, wrapS);
}

void Texture1D::setData(const void* data) {
	glBindTexture(GL_TEXTURE_1D, m_id);
	glTexSubImage1D(GL_TEXTURE_1D, 0, 0, m_width, m_format, m_type, data);
}

Texture2D::Texture2D(GLuint width, GLuint height, GLint internalFormat, GLenum format, GLenum type) 
	: TextureBase(internalFormat, format, type), m_width(width), m_height(height)
{
	glBindTexture(GL_TEXTURE_2D, m_id);

	setMinMagFiltering(GL_NEAREST, GL_NEAREST);
	setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);

	glTexImage2D(GL_TEXTURE_2D, 0, m_internalFormat, width, height, 0, m_format, m_type, nullptr);
	GL_CHECK_ERROR("Texture2D::Texture2D() - ERROR: ");
}

/**
 * Converts a number of floating point channels to an internal and external OpenGL format
 */
std::pair<GLint, GLenum> getFormats(const ImageF& img) {
	switch(img.getNumChannels()) {
	case 1:
		return std::make_pair(GL_R32F, GL_RED);
	case 2:
		return std::make_pair(GL_RG32F, GL_RG);
	case 3:
		return std::make_pair(GL_RGB32F, GL_RGB);
	case 4:
		return std::make_pair(GL_RGBA32F, GL_RGBA);
	}
	assert(false);
	return std::make_pair(0, 0);
}

Texture2D::Texture2D(const ImageF& img)
	: TextureBase(), m_width(img.getWidth()), m_height(img.getHeight())
{
	glBindTexture(GL_TEXTURE_2D, m_id);

	setMinMagFiltering(GL_NEAREST, GL_NEAREST);
	setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);

	auto formatPair = getFormats(img);
	m_internalFormat = std::get<0>(formatPair);
	m_format = std::get<1>(formatPair);
	m_type = GL_FLOAT;

	glTexImage2D(GL_TEXTURE_2D, 0, m_internalFormat, m_width, m_height, 0, m_format, m_type, img.data());
	GL_CHECK_ERROR("Texture2D::Texture2D() - ERROR: ");
}

void Texture2D::resize(GLuint width, GLuint height) {
	m_width = width;
	m_height = height;

	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexImage2D(GL_TEXTURE_2D, 0, m_internalFormat, width, height, 0, m_format, m_type, NULL);
}

void Texture2D::setMinMagFiltering(GLint min, GLint max) {
	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, max);
}

void Texture2D::setWrap(GLint wrapS, GLint wrapT) {
	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
}

void Texture2D::setData(const void* data) {
	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, m_format, m_type, data);
}

void Texture2D::setData(const ImageF& img) {
	setData((const void*) img.data());
}
