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
using std::array;

using uint = unsigned int;
using uint64 = uint64_t;

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
using mat3 = glm::mat3;
using mat4 = glm::mat4;
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;

/* Some common functions and macros */
#define CPVS_SAFE_DELETE(ptr) { if (ptr != NULL) delete ptr; }

#ifndef NDEBUG
	#define GL_CHECK_ERROR(X) checkGLErrors(X)
#else
	#define GL_CHECK_ERROR(X)
#endif

#define GL_ASSERT_NO_ERROR() assert(glGetError() == GL_NO_ERROR);

// Counts the number of set bits.
// Builtin exists for clang and gcc
#define POPCOUNT(x) __builtin_popcount(x)

/** Checks for OpenGL errors and outputs an error string */
extern void checkGLErrors(const std::string &str);

/** Returns true if the parameter is a power of two */
inline constexpr bool isPowerOfTwo(int x) {
	return !(x & (x - 1));
}

/** Computes the log of base 2 */
inline double log2(double x) {
	return log(x) * 1.44269504088896340736; //log(x) * log2(e)
}

inline double log8(double x) {
	return log(x) / 2.07944154167983592825; //log(x) / log(8)
}

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
