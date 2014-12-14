#ifndef FBO_H
#define FBO_H

#include "cpvs.h"
#include "Texture.h"

/** Represents a framebuffer object */
class Fbo
{
public:
	/**
	 * Creates a new Fbo with the given width and height.
	 * @param renderbuffer If true, a depth buffer will be attached to the Fbo.
	 */
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

	inline GLuint getWidth() const {
		return m_width;
	}

	inline GLuint getHeight() const {
		return m_height;
	}

	shared_ptr<Texture2D> getTexture(unsigned int index) const;

	shared_ptr<Texture2D> getDepthTexture() const {
		return m_depthTexture;
	}

	void addTexture(GLint internalFormat, GLenum format, GLenum type);

	void setDepthTexture(GLint internalFormat, GLenum format, GLenum type);

	void bindTexture(GLuint index, GLuint offset = 0) const;

	void bindTextures(GLuint offset = 0) const;

	size_t getNumTextures() const {
		return m_textures.size();
	}

	/**
	 * Returns an array of GL_COLOR_ATTACHMENT_i.
	 * @note Be aware that there could be no attachments!
	 */
	const vector<GLenum>& getColorAttachments() const {
		return m_colorAttachments;
	}

private:
	void attachRenderbuffer();

private:
	bool m_hasRenderbuffer;
	GLuint m_width, m_height;
	GLuint m_fbo;
	GLuint m_depth;

	shared_ptr<Texture2D> m_depthTexture;
	vector<GLenum> m_colorAttachments;
	vector<shared_ptr<Texture2D>> m_textures;
};

#endif // FBO_H
