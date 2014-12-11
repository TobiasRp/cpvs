#include "DeferredRenderer.h"

#include "Scene.h"

#include <iostream>
using namespace std;

DeferredRenderer::DeferredRenderer(int width, int height) 
	: m_fullscreenQuad(vec2(-1.0, -1.0), vec2(1.0, 1.0)), m_gBuffer(width, height, true)
{
	loadShaders();
}

void DeferredRenderer::loadShaders() {
	try {
		m_geometry.addShaderFromFile(GL_VERTEX_SHADER, "../shader/geometry.vert");
		m_geometry.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/geometry.frag");
		//m_geometry.addShaderFromFile(GL_VERTEX_SHADER, "../shader/forward.vert");
		//m_geometry.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/forward.frag");
		m_geometry.link();

		m_useDS.addShaderFromFile(GL_VERTEX_SHADER, "../shader/shade.vert");
		m_useDS.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/shade.frag");
		m_useDS.link();
	} catch (ShaderException& exc) {
		cout << exc.what() << endl;
		std::terminate();
	}
}

void DeferredRenderer::initFbos() {
	/* init gBuffer with FP texture for position + depth, and 8-bit textures for normal and color */
	m_gBuffer.bind();
	m_gBuffer.addTexture(GL_RGBA32F);
	m_gBuffer.addTexture(GL_RGBA8);
	m_gBuffer.addTexture(GL_RGBA8);
	m_gBuffer.release();
}

void DeferredRenderer::resize(int width, int height) {
	m_gBuffer.resize(width, height);
}

void DeferredRenderer::render(RenderProperties& properties, const Scene* scene) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderScene(properties, scene);

	doAllShading(properties, scene);
}


void DeferredRenderer::renderScene(RenderProperties& properties, const Scene* scene) {
	GL_CHECK_ERROR("DeferredRenderer::renderScene - begin");
	m_gBuffer.bind();
	m_gBuffer.clear();

	glEnable(GL_DEPTH_TEST);
	
	properties.setShaderProgram(&m_geometry);
	m_geometry.bind();

	GLenum buffers[] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
	};
	glDrawBuffers(3, buffers);

	/*TODO Set LIGHT */

	scene->render(properties);

	m_geometry.release();
	m_gBuffer.release();
	GL_CHECK_ERROR("DeferredRenderer::renderScene - end");
}

void DeferredRenderer::doAllShading(RenderProperties& properties, const Scene* scene) {
	GL_CHECK_ERROR("DeferredRenderer::doAllShading - begin");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_useDS.bind();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	m_gBuffer.bindTextures();

	m_fullscreenQuad.draw();

	m_useDS.release();
	GL_CHECK_ERROR("DeferredRenderer::doAllShading - end");
}
