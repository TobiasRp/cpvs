#ifndef RENDER_PROPERTIES_H
#define RENDER_PROPERTIES_H

#include "common.h"
#include "MatrixStack.h"
#include "ShaderProgram.h"

class RenderProperties {
public:
	RenderProperties(const ShaderProgram *shader, const mat4 &matV, const mat4 &matP)
		: m_shader(shader), V(matV), P(matP) {
			m_stack = std::make_unique<MatrixStack>(mat4(1.0f));
		}

	const ShaderProgram* getShaderProgram() const noexcept {
		return m_shader;
	}

	MatrixStack* getModelStack() const {
		return m_stack.get();
	}

	void setMatricesUBO(GLuint bo) {
		m_matricesUBO = bo;
	}

	void setModelInUniformBuffer() const {
		constexpr GLuint modelMatrixSlot = 2;
		const auto M = m_stack->top();

		glBindBuffer(GL_UNIFORM_BUFFER, m_matricesUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, modelMatrixSlot,  sizeof(float) * 16, glm::value_ptr(M));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	mat4 getMVP() const {
		return P * V * m_stack->top();
	}

	const mat4 V, P;

private:
	const ShaderProgram *m_shader;
	std::unique_ptr<MatrixStack> m_stack;

	GLuint m_matricesUBO;
};

#endif
