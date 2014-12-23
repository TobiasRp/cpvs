#ifndef TEXTURE_H
#define TEXTURE_H

#include "cpvs.h"
#include "Image.h"

class TextureBase {
public:
	TextureBase() {
		glGenTextures(1, &m_id);
	}

	TextureBase(GLint internalFormat, GLenum format, GLenum type)
		:m_internalFormat(internalFormat), m_format(format), m_type(type)
	{
		glGenTextures(1, &m_id);
	}

	virtual ~TextureBase() {
		glDeleteTextures(1, &m_id);
	}

	TextureBase(const TextureBase&) = delete;
	TextureBase& operator=(const TextureBase&) = delete;

	TextureBase(TextureBase&&) = default;
	TextureBase& operator=(TextureBase&&) = default;

	inline GLint getTextureId() const {
		return m_id;
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

	/**
	 * Binds the texture to the specified texture unit
	 */
	virtual void bindAt(GLint index) const = 0;

	/**
	 * Binds the texture to an image unit with the specified access parameter.
	 */
	inline void bindImageAt(GLuint index, GLenum access) const {
		glBindImageTexture(index, m_id, 0, GL_FALSE, 0, access, m_internalFormat);
	}

protected:
	GLuint m_id;
	GLint m_internalFormat;
	GLenum m_format;
	GLenum m_type;
};

class Texture1D : public TextureBase {
public:
	Texture1D(GLuint width, GLint internalFormat, GLenum format, GLenum type);

	virtual ~Texture1D() = default;

	Texture1D(const Texture1D&) = delete;
	Texture1D& operator=(const Texture1D&) = delete;

	Texture1D(Texture1D&&) = default;
	Texture1D& operator=(Texture1D&&) = default;

	inline GLuint getWidth() const;

	virtual void bindAt(GLint index) const override {
		glActiveTexture(GL_TEXTURE0 + index);
		glBindTexture(GL_TEXTURE_1D, m_id);
	}

	void setData(const void* data);

	void setWrap(GLint wrapS);

	void setMinMagFiltering(GLint min, GLint max);

protected:
	GLuint m_width;
};

class Texture2D : public TextureBase {
public:
	Texture2D(GLuint width, GLuint height, GLint internalFormat, GLenum format, GLenum type);

	/**
	 * Create a 32-bit floating points texture with one to four channels,
	 * according to the specified image.
	 */
	Texture2D(const ImageF& img);

	virtual ~Texture2D() = default;

	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;

	Texture2D(Texture2D&&) = default;
	Texture2D& operator=(Texture2D&&) = default;

	/**
	 * Changes the size of the texture.
	 * Be aware that this deletes and recreates the texture! All data is lost.
	 */
	void resize(GLuint width, GLuint height);

	void setMinMagFiltering(GLint min, GLint max);

	void setWrap(GLint wrapS, GLint wrapT);

	void setData(const void* data);
	
	void setData(const ImageF& img);

	virtual void bindAt(GLint index) const override {
		glActiveTexture(GL_TEXTURE0 + index);
		glBindTexture(GL_TEXTURE_2D, m_id);
	}

	inline GLuint getWidth() const {
		return m_width;
	}

	inline GLuint getHeight() const {
		return m_height;
	}

	inline bool isDepthTexture() const {
		return m_format == GL_DEPTH_COMPONENT;
	}

private:
	GLuint m_width, m_height;
};

#endif
