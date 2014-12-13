#include "Texture.h"

void setDefaultFiltering() {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

Texture2D::Texture2D(int width, int height, GLint internalFormat, GLenum format, GLenum type) 
	: m_internalFormat(internalFormat), m_format(format), m_type(type)
{
	glGenTextures(1, &m_id);
	glBindTexture(GL_TEXTURE_2D, m_id);

	setDefaultFiltering();
	resize(width, height);
	GL_CHECK_ERROR("Texture2D::Texture2D() - ERROR: ");
}

Texture2D::~Texture2D() {
	glDeleteTextures(1, &m_id);
}

void Texture2D::resize(int width, int height) {
	m_width = width;
	m_height = height;

	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexImage2D(GL_TEXTURE_2D, 0, m_internalFormat, width, height, 0, m_format, m_type, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::bindAt(GLint index) const {
	glActiveTexture(GL_TEXTURE0 + index);
	glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture2D::setMinMagFiltering(GLint min, GLint max) const {
	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, max);
	glBindTexture(GL_TEXTURE_2D, 0);
}
