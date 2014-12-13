#ifndef FBO_H
#define FBO_H

#include "cpvs.h"
#include "Texture.h"

/** Represents a framebuffer object */
class Fbo
{
public:
	Fbo(GLuint width, GLuint height, bool renderbuffer);
	~Fbo();

	Fbo(const Fbo&) = delete;
	Fbo& operator=(const Fbo&) = delete;

	Fbo(Fbo&&) = default;
	Fbo& operator=(Fbo&&) = default;

	void clear();

	void bind() const;

	void release() const;

	void resize(int width, int height);

	shared_ptr<Texture2D> getTexture(unsigned int index) const;

	void addTexture(GLint internalFormat, GLenum format, GLenum type);

	void bindTexture(GLuint index, GLuint offset = 0) const;

	void bindTextures(GLuint offset = 0) const;

	size_t getNumTextures() const {
		return m_textures.size();
	}

	const vector<GLenum>& getColorAttachments() const {
		return m_colorAttachments;
	}

private:
	bool m_hasRenderbuffer;
	GLuint m_width, m_height;
	GLuint m_fbo;
	GLuint m_depth;

	vector<GLenum> m_colorAttachments;
	vector<shared_ptr<Texture2D>> m_textures;
};

#endif // FBO_H
