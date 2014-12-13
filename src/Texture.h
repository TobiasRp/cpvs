#ifndef TEXTURE_H
#define TEXTURE_H

#include "cpvs.h"

class Texture2D {
public:
	Texture2D(int width, int height, GLint internalFormat, GLenum format, GLenum type);
	~Texture2D();

	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;

	Texture2D(Texture2D&&) = default;
	Texture2D& operator=(Texture2D&&) = default;


	inline GLint getTextureId() const {
		return m_id;
	}

	void resize(int width, int height);

	void bindAt(GLint index) const;

	void setMinMagFiltering(GLint min, GLint max) const;

private:
	GLuint m_id;
	GLint m_internalFormat;
	GLenum m_format;
	GLenum m_type;

	int m_width, m_height;
};

#endif
