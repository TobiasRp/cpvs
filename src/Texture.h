#ifndef TEXTURE_H
#define TEXTURE_H

#include "cpvs.h"
#include "Image.h"

class Texture2D {
public:
	Texture2D(GLuint width, GLuint height, GLint internalFormat, GLenum format, GLenum type);

	/**
	 * Create a 32-bit floating points texture with one to four channels,
	 * according to the specified image.
	 */
	Texture2D(const ImageF& img);

	~Texture2D();

	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;

	Texture2D(Texture2D&&) = default;
	Texture2D& operator=(Texture2D&&) = default;

	inline GLint getTextureId() const {
		return m_id;
	}

	/**
	 * Changes the size of the texture.
	 * Be aware that this deletes and recreates the texture! All data is lost.
	 */
	void resize(GLuint width, GLuint height);

	/**
	 * Binds the texture to the specified texture unit
	 */
	void bindAt(GLint index) const;

	void setMinMagFiltering(GLint min, GLint max);

	void setWrap(GLint wrapS, GLint wrapT);

	void setData(const void* data);
	
	void setData(const ImageF& img);

	inline GLuint getWidth() const {
		return m_width;
	}

	inline GLuint getHeight() const {
		return m_height;
	}

	inline GLint getInternalFormat() const {
		return m_internalFormat;
	}

	inline GLenum getFormat() const {
		return m_format;
	}

	inline GLenum getType() const {
		return m_type;
	}

	inline bool isDepthTexture() const {
		return m_format == GL_DEPTH_COMPONENT;
	}

private:
	GLuint m_id;
	GLint m_internalFormat;
	GLenum m_format;
	GLenum m_type;

	GLuint m_width, m_height;
};

#endif
