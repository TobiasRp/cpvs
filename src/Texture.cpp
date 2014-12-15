#include "Texture.h"

Texture2D::Texture2D(GLuint width, GLuint height, GLint internalFormat, GLenum format, GLenum type) 
	: m_internalFormat(internalFormat), m_format(format), m_type(type), m_width(width), m_height(height)
{
	glGenTextures(1, &m_id);
	glBindTexture(GL_TEXTURE_2D, m_id);

	setMinMagFiltering(GL_NEAREST, GL_NEAREST);
	setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);

	glTexImage2D(GL_TEXTURE_2D, 0, m_internalFormat, width, height, 0, m_format, m_type, NULL);
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
	: m_type(GL_FLOAT), m_width(img.getWidth()), m_height(img.getHeight())
{
	glGenTextures(1, &m_id);
	glBindTexture(GL_TEXTURE_2D, m_id);

	setMinMagFiltering(GL_NEAREST, GL_NEAREST);
	setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);

	auto formatPair = getFormats(img);
	m_internalFormat = std::get<0>(formatPair);
	m_format = std::get<1>(formatPair);

	glTexImage2D(GL_TEXTURE_2D, 0, m_internalFormat, m_width, m_height, 0, m_format, m_type, img.data());
	GL_CHECK_ERROR("Texture2D::Texture2D() - ERROR: ");
}

Texture2D::~Texture2D() {
	glDeleteTextures(1, &m_id);
}

void Texture2D::resize(GLuint width, GLuint height) {
	m_width = width;
	m_height = height;

	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexImage2D(GL_TEXTURE_2D, 0, m_internalFormat, width, height, 0, m_format, m_type, NULL);
}

void Texture2D::bindAt(GLint index) const {
	glActiveTexture(GL_TEXTURE0 + index);
	glBindTexture(GL_TEXTURE_2D, m_id);
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
