#include "Fbo.h"

#include <iostream>
using namespace std;

Fbo::Fbo(GLuint width, GLuint height, bool renderbuffer)
	: m_hasRenderbuffer(renderbuffer), m_width(width), m_height(height)
{
	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	if (renderbuffer) {
		attachRenderbuffer();
	}

	auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (renderbuffer)
		assert(status == GL_FRAMEBUFFER_COMPLETE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GL_CHECK_ERROR("Fbo::Fbo() - ERROR: ");
}

Fbo::~Fbo() {
	if (m_hasRenderbuffer)
		glDeleteRenderbuffers(1, &m_depth);

	glDeleteFramebuffers(1, &m_fbo);
}

void Fbo::attachRenderbuffer() {
	glGenRenderbuffers(1, &m_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depth);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, m_width, m_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth);
}

void Fbo::clear() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Fbo::bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void Fbo::release() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Fbo::resize(int width, int height)
{
	m_width = width;
	m_height = height;

	for (auto& tex : m_textures) {
		tex->resize(width, height);
	}
	if (m_hasRenderbuffer) {
		glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);
	}
	if (m_depthTexture.get() != nullptr) {
		m_depthTexture->resize(width, height);
	}
	GL_CHECK_ERROR("Fbo::resize() - ERROR: ");
}

shared_ptr<Texture2D> Fbo::getTexture(unsigned int index) const {
	assert(index < m_textures.size());
	return m_textures[index];
}

void Fbo::addTexture(GLint internalFormat, GLenum format, GLenum type) {
	auto tex = std::make_shared<Texture2D>(m_width, m_height, internalFormat, format, type);

	m_textures.push_back(tex);
	GLuint attachment = m_textures.size() - 1;

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment,
		GL_TEXTURE_2D, tex->getTextureId(), 0);
	m_colorAttachments.push_back(GL_COLOR_ATTACHMENT0 + m_colorAttachments.size());
}

void Fbo::setDepthTexture(GLint internalFormat, GLenum format, GLenum type) {
	assert(m_depthTexture.get() == nullptr && m_hasRenderbuffer == false); // only set depth once!

	m_depthTexture = std::make_shared<Texture2D>(m_width, m_height, internalFormat, format, type);
	assert(m_depthTexture->isDepthTexture());

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexture->getTextureId(), 0);
}

void Fbo::bindTexture(GLuint index, GLuint offset) const {
	assert(index < m_textures.size());

	m_textures[index]->bindAt(offset);
}

void Fbo::bindTextures(GLuint offset) const {
	for (unsigned int i = 0; i < m_textures.size(); ++i) {
		m_textures[i]->bindAt(offset + i);
	}
}
