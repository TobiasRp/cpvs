#ifndef MATRIX_STACK_H
#define MATRIX_STACK_H

#include "cpvs.h"
#include <stack>

class MatrixStack {
public:
	MatrixStack(mat4 matrix) {
   		m_stack.push(matrix);
	}

	mat4 top() const {
		return m_stack.top();
	}

	void push(mat4 mat) {
		auto oldM = m_stack.top();
		auto newM = oldM * mat;
		m_stack.push(newM);
	}

	void pop() {
		m_stack.pop();
	}

private:
	std::stack<mat4> m_stack;
};

#endif
