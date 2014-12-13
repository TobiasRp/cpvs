#include "Shader.h"

#include <fstream>
#include <vector>

Shader::Shader(GLenum type)
	: m_type(type)
{
	m_id = glCreateShader(m_type);
}

Shader::~Shader()
{
	glDeleteShader(m_id);
}

void Shader::compileSource(const char *source)
{
	glShaderSource(m_id, 1, &source, NULL);
	glCompileShader(m_id);

	GLint status;
	glGetShaderiv(m_id, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		throw ShaderException(log());
	}
}

void Shader::compileFile(const char *file)
{
	using namespace std;

	string code;
	ifstream stream(file, std::ios::in);
	if (!stream.is_open())
		throw FileNotFound(file);

	string line;
	while (getline(stream, line))
		code += "\n" + line;
	code += '\0';

	compileSource(code.c_str());
}


bool Shader::isCompiled() const
{
	GLint success = 0;
	glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);
	return success != GL_FALSE;
}

std::string Shader::log() const
{
	GLsizei length;
	glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &length);

	std::vector<char> errorLog(length + 1);
	glGetShaderInfoLog(m_id, length, NULL, &errorLog[0]);

	return std::string(&errorLog[0], length + 1);
}
