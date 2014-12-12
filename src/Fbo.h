#ifndef FBO_H
#define FBO_H

#include "cpvs.h"

/** Represents a framebuffer object */
class Fbo
{
private:
	struct TexTarget {
		GLuint id;
		GLint format;
	};

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

	GLuint getTexture(unsigned int index) const;

	void addTexture(GLint format);

	void bindTexture(GLuint index, GLuint offset = 0) const;

	void bindTextures(GLuint offset = 0) const;

	size_t getNumTextures() const {
		return textures.size();
	}

	const vector<GLenum>& getColorAttachments() const {
		return m_colorAttachments;
	}

	void setTextureFiltering(unsigned int index, GLint min, GLint max, GLuint offset = 0);

private:
	bool m_hasRenderbuffer;
	GLuint m_width, m_height;
	GLuint m_fbo;
	GLuint m_depth;

	vector<GLenum> m_colorAttachments;
	vector<TexTarget> textures;
};

#endif // FBO_H
