/*
* Copyright Tobias Rapp
* 16.05.2014
*/
#ifndef AW_SHADERPROGRAM_H
#define AW_SHADERPROGRAM_H

#include "cpvs.h"
#include "Shader.h"
#include <unordered_map>

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

	GLint getAttribLocation(const string& attribute) const;

	/** Adds an uniform name to the program, so it's location can be accessed.
	 * Use operator[] to access the location.
	 */ 
	void addUniform(const string& uniform);

	/** Returns the location of the specified uniform */
	GLint operator[](const string& uniform) {
		return m_uniformLocations[uniform];
	}

	/** operator[] for rvalue types */
	GLint operator[](string&& uniform) {
		return m_uniformLocations[std::move(uniform)];
	}

private:
	using StringMap = std::unordered_map<string, GLint>;

	StringMap m_uniformLocations;
	GLuint m_id;
};
#endif // SHADERPROGRAM_H
