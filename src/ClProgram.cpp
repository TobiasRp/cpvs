#include "ClProgram.h"

#define __CL_ENABLE_EXCEPTIONS

#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>
#include <algorithm>

#include <GL/glx.h>

using namespace std;

static cl::Context context;
static vector<cl::Device> devices;
static vector<cl::CommandQueue> commandQueues;

const char* getErrorString(cl_int err) {
	switch(err) {
	case CL_INVALID_VALUE:
		return "CL_INVALID_VALUE";
	case CL_OUT_OF_HOST_MEMORY:
		return "CL_OUT_OF_HOST_MEMORY";
	case CL_PLATFORM_NOT_FOUND_KHR:
		return "CL_PLATFORM_NOT_FOUND_KHR";
	}
	return "";
}

void checkCLErrors(cl_int err, const char* name) {
	if (err != CL_SUCCESS) {
		std::cerr << "Error: " << name << " (" << getErrorString(err) << ")" << std::endl;
		std::terminate();
	}
}

/* Linux implementation for creating a shared context with OpenGL */
void createSharedContext(const cl::Platform& pl) {
	cl_context_properties cps[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
		CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)pl()
	};
	context = cl::Context(CL_DEVICE_TYPE_GPU, cps);
}

void createNewContext(const cl::Platform& pl) {
	cl_context_properties cps[] = {
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)pl(),
		0
	};
	context = cl::Context(CL_DEVICE_TYPE_GPU, cps);
}

void initializeCL(bool useOpenGL) {
	vector<cl::Platform> platformList;
	cl::Platform::get(&platformList);

	for (const auto& pl : platformList) {
		string vendor;
		pl.getInfo((cl_platform_info)CL_PLATFORM_VENDOR, &vendor);
		cout << "Platform is by: " << vendor << "\n";
	}
	cl::Platform defaultPlatform = platformList[0];

	defaultPlatform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

	for (const auto& dev : devices) {
		string version;
		dev.getInfo((cl_platform_info)CL_DEVICE_VERSION, &version);
		cout << "OpenCL device version is: " << version << "\n";
	}

	if (useOpenGL)
		createSharedContext(defaultPlatform);
	else
		createNewContext(defaultPlatform);

	for_each(devices.begin(), devices.end(), [ctx = context](auto dev)
			{ commandQueues.push_back(cl::CommandQueue(ctx, dev)); });
}

ClProgram::ClProgram(string sourceFile) {
	auto sources = readSource(sourceFile);
	m_program = cl::Program(context, sources);
	m_program.build(devices);
}

cl::Program::Sources ClProgram::readSource(const std::string& sourceFile) {
	ifstream file(sourceFile);
	string code(istreambuf_iterator<char>(file), (istreambuf_iterator<char>()));
	return cl::Program::Sources(1, make_pair(code.c_str(), code.length()+1));
}

cl::Context ClProgram::getContext() const {
	return context;
}

cl::CommandQueue ClProgram::getCommandQueue(unsigned deviceNr) const {
	assert(deviceNr < commandQueues.size());
	return commandQueues[deviceNr];
}
