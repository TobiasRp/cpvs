/*
* Copyright Tobias Rapp
* 16.05.2014
*/
#ifndef AW_SHADERPROGRAM_H
#define AW_SHADERPROGRAM_H

#include "Shader.h"

/**
 * @brief The ShaderProgram class encapsulates a GLSL program.
 */
class ShaderProgram
{
public:
	ShaderProgram();
	~ShaderProgram();

	/* Disable copying */
	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;

	/* ...but allow moving */
	ShaderProgram(ShaderProgram&& rhs) : m_id(rhs.m_id) { }
	ShaderProgram& operator=(ShaderProgram&& rhs) {
		m_id = rhs.m_id;
		return *this;
	}

	void addShader(Shader &shader);

	void addShaderFromSource(GLenum type, const char *source);

	void addShaderFromFile(GLenum type, const char *file);

	void link();

	bool isLinked() const;

	std::string log() const;

	void bind() const;

	void release() const;

	/** This is not done by the destructor, because it would result in errors if the
	 *  object is declared static/global
	 */
	void deleteProgram();

	GLint getAttribLocation(const char *name) const;

	/**
	* @brief getUniformLocation Returns the index of the uniform variable 'name',
	* associated with this shader program.
	*
	* @param name A null-terminated character string with no spaces.
	* @return The index or a value of minus one, if 'name' does not correspond to
	* a uniform variable in this shader program.
	*
	* @note The returned value will not change unless the shader program is relinked.
	*/
	GLint getUniformLocation(const char *name) const;

	void setUniformMatrix4fv(GLuint location, const GLfloat* values) const;
	void setUniformMatrix4fv(const char *name, const GLfloat* values) const;

	void setUniformMatrix3fv(GLuint location, const GLfloat* values) const;
	void setUniformMatrix3fv(const char *name, const GLfloat* values) const;

	void setUniform4f(GLuint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const;
	void setUniform4f(const char *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const;

	void setUniform3fv(GLuint location, const GLfloat* values) const;
	void setUniform3fv(const char *name, const GLfloat* values) const;

	void setUniform3f(GLuint location, GLfloat x, GLfloat y, GLfloat z) const;
	void setUniform3f(const char *name, GLfloat x, GLfloat y, GLfloat z) const;

	void setUniform2f(GLuint location, GLfloat x, GLfloat y) const;
	void setUniform2f(const char *name, GLfloat x, GLfloat y) const;

	void setUniform1i(GLuint location, GLint x) const;
	void setUniform1i(const char *name, GLint x) const;

	void setUniform1f(GLuint location, GLfloat x) const;
	void setUniform1f(const char *name, GLfloat x) const;

private:
	GLuint m_id;
};
#endif // SHADERPROGRAM_H
