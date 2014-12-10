#include "common.h"
#include <iostream>

using namespace std;

void checkGLErrors(const std::string &str)
{
    switch(glGetError()) {
        case GL_NO_ERROR:
            break;
        case GL_INVALID_ENUM:
            cerr << str << "Invalid enum." << endl;
            break;
        case GL_INVALID_VALUE:
			cerr << str << "Invalid value." << endl;
            break;
        case GL_INVALID_OPERATION:
			cerr << str << "Invalid operation." << endl;
            break;
        case GL_STACK_OVERFLOW:
			cerr << str << "Stack overflow." << endl;
            break;
        case GL_STACK_UNDERFLOW:
			cerr << str << "Stack underflow." << endl;
            break;
        case GL_OUT_OF_MEMORY:
			cerr << str << "Out of memory." << endl;
            break;
        default:
			cerr << str << "Unknown error." << endl;
            break;
     }
}
