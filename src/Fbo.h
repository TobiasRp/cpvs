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

	void clear();

	void bind();

	void release();

	void resize(int width, int height);

	GLuint getTexture(unsigned int index);

	void addTexture(GLint format);

	void bindTexture(GLuint index, GLuint offset = 0);

	void bindTextures(GLuint offset = 0);

	size_t getNumTextures() const {
		return textures.size();
	}

	void setTextureFiltering(unsigned int index, GLint min, GLint max, GLuint offset = 0);

private:
	bool m_hasRenderbuffer;
	GLuint m_width, m_height;
	GLuint m_fbo;
	GLuint m_depth;

	std::vector<TexTarget> textures;
};

#endif // FBO_H
