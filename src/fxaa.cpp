#include "fxaa.h"

#include "Fbo.h"
#include "Quad.h"

#include <iostream>

void FXAA::initialize(int width, int height) {
	try {
		m_prog.addShaderFromFile(GL_VERTEX_SHADER, "../shader/shade.vert");
		m_prog.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/FXAA.frag");
		m_prog.link();
	} catch (ShaderException& exc) {
		std::cerr << exc.what() << std::endl;
		std::terminate();
	}

    m_prog.bind();
	m_prog.addUniform("renderMode");
	m_prog.addUniform("pixelSize");

	glUniform1i(m_prog["renderMode"], 0);
    resize(width, height);
	m_prog.release();
    GL_CHECK_ERROR("FXAA::initialize() - ERROR: ");
}

void FXAA::resize(int width, int height) {
    m_prog.bind();
	glUniform2f(m_prog["pixelSize"], 1 / (float) width, 1 / (float) height);
    m_prog.release();
}

void FXAA::render(const Fbo& gBuffer, Fbo& inBuffer, const Quad& fullScreen) {
    (void)gBuffer; // we do not need the gBuffer in this effect
    m_prog.bind();

    inBuffer.bindTexture(0);
	auto tex = inBuffer.getTexture(0);
	tex->setMinMagFiltering(GL_LINEAR, GL_LINEAR);

    fullScreen.draw();

    m_prog.release();
    GL_CHECK_ERROR("FXAA::render - Error ");
}

