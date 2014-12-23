/*
* By Tobias Rapp
* 16.05.2014
*/
#ifndef AW_SHADER_H
#define AW_SHADER_H

#include "cpvs.h"

class ShaderException {
public:
	ShaderException(std::string msg)
		: m_msg(msg) { }

	std::string what() const {
		return m_msg;
	}

	std::string where() const {
		return m_file;
	}

	std::string m_msg;
	std::string m_file;
};

class Shader
{
public:
	Shader(GLenum type);
	~Shader();

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	Shader(Shader&& rhs) : m_type(rhs.m_type), m_id(rhs.m_id) { }
	Shader& operator=(Shader&& rhs) {
		m_type = rhs.m_type;
		m_id = rhs.m_id;
		return *this;
	}

	void compileSource(const char *source);

	void compileFile(const char *file);

	std::string log() const;

	bool isCompiled() const;

	GLuint getShaderID() const { return m_id; }

private:
	GLenum m_type;
	GLuint m_id;
};

#endif // SHADER_H
