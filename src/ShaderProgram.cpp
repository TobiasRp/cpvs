#include "ShaderProgram.h"

#include <vector>
#include <cassert>
#include <iostream>

ShaderProgram::ShaderProgram()
{
	m_id = glCreateProgram();
}

ShaderProgram::~ShaderProgram() {
	glDeleteProgram(m_id);
}

void ShaderProgram::addShader(Shader &shader)
{
	assert(shader.isCompiled());

	glAttachShader(m_id, shader.getShaderID());
}

void ShaderProgram::addShaderFromSource(GLenum type, const char *source)
{
	Shader shader(type);
	shader.compileSource(source);
	addShader(shader);
}

void ShaderProgram::addShaderFromFile(GLenum type, const char *file)
{
	Shader shader(type);
	shader.compileFile(file);
	addShader(shader);
}

void ShaderProgram::link()
{
	glLinkProgram(m_id);
}

bool ShaderProgram::isLinked() const
{
	GLint status;
	glGetProgramiv(m_id, GL_LINK_STATUS, &status);
	return status == GL_TRUE;
}

std::string ShaderProgram::log() const
{
	GLsizei length;
	glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);

	std::vector<char> errorLog(length + 1);
	glGetProgramInfoLog(m_id, length, NULL, &errorLog[0]);

	return std::string(&errorLog[0], length + 1);
}

void ShaderProgram::bind() const
{
	glUseProgram(m_id);
}

void ShaderProgram::release() const
{
	glUseProgram(0);
}

GLint ShaderProgram::getAttribLocation(const char *name) const
{
	return glGetAttribLocation(m_id, name);
}

GLint ShaderProgram::getUniformLocation(const char *name) const {
	return glGetUniformLocation(m_id, name);
}

void ShaderProgram::setUniformMatrix4fv(GLuint location, const GLfloat *values) const
{
	glUniformMatrix4fv(location, 1, GL_FALSE, values);
}

void ShaderProgram::setUniformMatrix4fv(const char *name, const GLfloat *values) const
{
	setUniformMatrix4fv(getUniformLocation(name), values);
}

void ShaderProgram::setUniformMatrix3fv(GLuint location, const GLfloat *values) const
{
	glUniformMatrix3fv(location, 1, GL_FALSE, values);
}

void ShaderProgram::setUniformMatrix3fv(const char *name, const GLfloat *values) const
{
	setUniformMatrix3fv(getUniformLocation(name), values);
}

void ShaderProgram::setUniform4f(GLuint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const
{
	glUniform4f(location, x, y, z, w);
}

void ShaderProgram::setUniform4f(const char *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const
{
	setUniform4f(getUniformLocation(name), x, y, z, w);
}

void ShaderProgram::setUniform3fv(GLuint location, const GLfloat *values) const
{
	glUniform3fv(location, 1, values);
}

void ShaderProgram::setUniform3fv(const char *name, const GLfloat *values) const
{
	setUniform3fv(getUniformLocation(name), values);
}

void ShaderProgram::setUniform3f(GLuint location, GLfloat x, GLfloat y, GLfloat z) const
{
	glUniform3f(location, x, y, z);
}

void ShaderProgram::setUniform3f(const char *name, GLfloat x, GLfloat y, GLfloat z) const
{
	setUniform3f(getUniformLocation(name), x, y, z);
}

void ShaderProgram::setUniform2f(GLuint location, GLfloat x, GLfloat y) const
{
	glUniform2f(location, x, y);
}

void ShaderProgram::setUniform2f(const char *name, GLfloat x, GLfloat y) const
{
	setUniform2f(getUniformLocation(name), x, y);
}

void ShaderProgram::setUniform1i(GLuint location, GLint x) const
{
	glUniform1i(location, x);
}

void ShaderProgram::setUniform1i(const char *name, GLint x) const
{
	setUniform1i(getUniformLocation(name), x);
}

void ShaderProgram::setUniform1f(GLuint location, GLfloat x) const
{
	glUniform1f(location, x);
}

void ShaderProgram::setUniform1f(const char *name, GLfloat x) const
{
	setUniform1f(getUniformLocation(name), x);
}
