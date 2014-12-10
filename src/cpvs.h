#ifndef CPVS_H
#define CPVS_H

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#include "common.h"


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
