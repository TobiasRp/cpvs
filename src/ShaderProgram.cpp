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
	m_uniformLocations.clear();
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

void ShaderProgram::addUniform(const string& uniform) {
	m_uniformLocations[uniform] = glGetUniformLocation(m_id, uniform.c_str());
}

GLint ShaderProgram::getAttribLocation(const string& attribute) const
{
	return glGetAttribLocation(m_id, attribute.c_str());
}
