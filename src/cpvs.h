#ifndef CPVS_H
#define CPVS_H

#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <exception>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

using std::string;
using std::vector;
using std::shared_ptr;
using std::unique_ptr;

typedef unsigned int uint;

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
using mat4 = glm::mat4;
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

/* Some common functions and macros */
#define CPVS_SAFE_DELETE(ptr) { if (ptr != NULL) delete ptr; }

#ifndef NDEBUG
	#define GL_CHECK_ERROR(X) checkGLErrors(X)
#else
	#define GL_CHECK_ERROR(X)
#endif

#define GL_ASSERT_NO_ERROR() assert(glGetError() == GL_NO_ERROR);

/* Checks for OpenGL errors and outputs an error string */
extern void checkGLErrors(const std::string &str);


/* Some exception types */

class FileNotFound : std::exception {
public:
	FileNotFound(const char *msg) noexcept : m_msg(msg) { }
	FileNotFound() noexcept : m_msg("File not found\n") { }
	virtual const char* what() const noexcept override {
		return m_msg;
	}
private:
	const char *m_msg;
};

class LoadFileException : std::exception {
public:
	LoadFileException(const char *str) noexcept
		: m_msg(str) { }
	LoadFileException() noexcept : m_msg("Could not load/parse file\n") { }
	virtual const char* what() const noexcept override {
		return m_msg;
	}
private:
	const char *m_msg;
};


#endif
