#ifndef RENDER_PROPERTIES_H
#define RENDER_PROPERTIES_H

#include "common.h"
#include "MatrixStack.h"
#include "ShaderProgram.h"

class RenderProperties {
public:
	RenderProperties(const mat4 &matV, const mat4 &matP)
		: V(matV), P(matP)
	{
			m_stack = std::make_unique<MatrixStack>(mat4(1.0f));
	}

	void setShaderProgram(const ShaderProgram *shader) {
		m_shader = shader;
	}

	const ShaderProgram* getShaderProgram() const noexcept {
		return m_shader;
	}

	MatrixStack* getModelStack() const {
		return m_stack.get();
	}

	mat4 getMVP() const {
		return P * V * m_stack->top();
	}

	mat4 getNormalMatrix() const {
		return glm::transpose(glm::inverse(getMVP()));
	}

	const mat4 V, P;

private:
	const ShaderProgram* m_shader;
	std::unique_ptr<MatrixStack> m_stack;
};

#endif
