#ifndef RENDER_PROPERTIES_H
#define RENDER_PROPERTIES_H

#include "cpvs.h"
#include "MatrixStack.h"
#include "ShaderProgram.h"

class RenderProperties {
public:
	RenderProperties(const mat4 &matV, const mat4 &matP)
		: V(matV), P(matP), m_renderMaterials(false)
	{
			m_stack = std::make_unique<MatrixStack>(mat4(1.0f));
	}

	void setShaderProgram(ShaderProgram *shader) noexcept {
		m_shader = shader;
	}

	ShaderProgram* getShaderProgram() const noexcept {
		return m_shader;
	}

	MatrixStack* getModelStack() const {
		return m_stack.get();
	}

	mat4 getMVP() const {
		return P * V * m_stack->top();
	}

	mat4 getMV() const {
		return V * m_stack->top();
	}
	
	mat4 getM() const {
		return m_stack->top();
	}

	mat3 getNormalMatrix() const {
		mat3 M = mat3(getM());
		return glm::transpose(glm::inverse(M));
	}

	void setRenderingOfMaterials(bool value) noexcept {
		m_renderMaterials = value;
	}

	bool renderMaterials() const noexcept {
		return m_renderMaterials;
	}

	mat4 V, P;
private:
	ShaderProgram* m_shader;
	std::unique_ptr<MatrixStack> m_stack;

	bool m_renderMaterials;
};

#endif
