#include "Fbo.h"

void createTex(GLuint *tex, GLuint width, GLuint height, GLint format)
{
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_2D, *tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	GL_CHECK_ERROR("Fbo.cpp:createTex() - ERROR: ");
}

Fbo::Fbo(GLuint width, GLuint height, bool renderbuffer)
	: m_isBound(false), m_hasRenderbuffer(renderbuffer), m_width(width), m_height(height)
{
	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	if (renderbuffer) {
		glGenRenderbuffers(1, &m_depth);
		glBindRenderbuffer(GL_RENDERBUFFER, m_depth);

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth);
	}

	GLenum status;
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (renderbuffer)
		assert(status == GL_FRAMEBUFFER_COMPLETE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GL_CHECK_ERROR("Fbo::Fbo() - ERROR: ");
}

Fbo::~Fbo()
{
	for (auto tex : textures) {
		glDeleteTextures(1, &tex.id);
	}

	if (m_hasRenderbuffer)
		glDeleteRenderbuffers(1, &m_depth);

	glDeleteFramebuffers(1, &m_fbo);
}

void Fbo::clear() {
	assert(m_isBound);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Fbo::bind() {
	m_isBound = true;
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void Fbo::release() {
	m_isBound = false;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void resizeTex(GLuint tex, GLuint width, GLuint height, GLint format)
{
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	GL_CHECK_ERROR("Fbo::resizeTex() - ERROR: ");
}

void Fbo::resize(int width, int height)
{
	m_width = width;
	m_height = height;

	for (auto tex : textures) {
		resizeTex(tex.id, width, height, tex.format);
	}
	if (m_hasRenderbuffer) {
		glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
	}
	GL_CHECK_ERROR("Fbo::resize() - ERROR: ");
}

GLuint Fbo::getTexture(unsigned int index) {
	assert(index < textures.size());
	return textures[index].id;
}

void Fbo::addTexture(GLint format)
{
	bool release;
	if (m_isBound)
		release = false;
	else {
		bind();
		release = true;
	}

	TexTarget tgt;
	tgt.format = format;
	createTex(&tgt.id, m_width, m_height, format);
	textures.push_back(tgt);

	GLuint attachment = textures.size() - 1;
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment,
		GL_TEXTURE_2D, tgt.id, 0);

	if (release)
		this->release();
}

void Fbo::bindTexture(GLuint index, GLuint offset)
{
	assert(index < textures.size());

	glActiveTexture(GL_TEXTURE0 + offset);
	glBindTexture(GL_TEXTURE_2D, textures[index].id);
}

void Fbo::bindTextures(GLuint offset)
{
	for (unsigned int i = 0; i < textures.size(); ++i) {
		glActiveTexture(GL_TEXTURE0 + offset + i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}
}

void Fbo::setTextureFiltering(unsigned int index, GLint min, GLint max, GLuint offset)
{
	bindTexture(index + offset);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, max);
}
