#ifndef BUFFER_H
#define BUFFER_H

#include "cpvs.h"

/**
 * A shader storage buffer represents memory on the GPU which can be acced from a shader.
 */
class SSBO {
public:
	template<typename T>
	SSBO(const vector<T>& data, GLenum usage) {
		glGenBuffers(1, &m_bo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_bo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data[0]) * data.size(), data.data(), usage);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	~SSBO() {
		glDeleteBuffers(1, &m_bo);
	}

	SSBO(const SSBO&) = delete;
	SSBO& operator=(const SSBO&) = delete;

	SSBO(SSBO&&) = default;
	SSBO& operator=(SSBO&&) = default;

	/**
	 * Bind the ssbo at the specified index.
	 */
	inline void bindAt(GLuint index) {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_bo);
	}

private:
	GLuint m_bo;
};

#endif
