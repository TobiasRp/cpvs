#ifndef CL_PROGRAM_H
#define CL_PROGRAM_H

#include <CL/cl.hpp>

#include <memory>
using std::unique_ptr;

extern void checkCLErrors(cl_int err, const char* name);

/**
 * Intializes all OpenCL stuff.
 */
extern void initializeCL(bool useOpenGL = true);

/**
 * Wraps a cl::Program, but also takes care of context, devices, command queues, ... in a simple way.
 *
 * @note initializeCL has to be called before this can be used.
 */
class ClProgram {
private:
	ClProgram(std::string sourceFile);

	cl::Program::Sources readSource(const std::string& sourceFile);

public:
	template<typename T>
	static unique_ptr<ClProgram> create(T&& sourceFile) {
		auto ptr = new ClProgram(std::forward<T>(sourceFile));
		return unique_ptr<ClProgram>(ptr);
	}

	/**
	 * Returns a kernel from the CL program.
	 */
	template<typename T>
	cl::Kernel getKernel(T&& name) const {
		return cl::Kernel(m_program, std::forward<T>(name));
	}

	cl::Context getContext() const;

	cl::CommandQueue getCommandQueue(unsigned deviceNr = 0) const;

private:
	cl::Program m_program;
};

#endif
