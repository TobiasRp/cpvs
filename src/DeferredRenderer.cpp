#include "DeferredRenderer.h"

#include "Scene.h"

#include <iostream>
using namespace std;

DeferredRenderer::DeferredRenderer(const DirectionalLight& light, int width, int height) 
	: m_fullscreenQuad(vec2(-1.0, -1.0), vec2(1.0, 1.0)), m_gBuffer(width, height, true), m_dirLight(light)
{
	loadShaders();
	initFbos();
}

void DeferredRenderer::loadShaders() {
	try {
		m_geometry.addShaderFromFile(GL_VERTEX_SHADER, "../shader/geometry.vert");
		m_geometry.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/geometry.frag");
		m_geometry.link();

		m_shade.addShaderFromFile(GL_VERTEX_SHADER, "../shader/shade.vert");
		m_shade.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/shade.frag");
		m_shade.link();
	} catch (ShaderException& exc) {
		cout << exc.what() << endl;
		std::terminate();
	}

	/* Add uniforms to the shader */
	m_geometry.addUniform("MVP");
	m_geometry.addUniform("NormalMatrix");
	m_geometry.addUniform("material.shininess");
	m_geometry.addUniform("material.diffuse_color");

	m_shade.addUniform("light.color");
	m_shade.addUniform("light.direction");
}

void DeferredRenderer::initFbos() {
	/* init gBuffer with FP texture for position + depth, and 8-bit textures for normal and color */
	m_gBuffer.bind();
	m_gBuffer.addTexture(GL_RGBA32F);
	m_gBuffer.addTexture(GL_RGBA8);
	m_gBuffer.addTexture(GL_RGBA8);

	glDrawBuffers(3, m_gBuffer.getColorAttachments().data());
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

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	
	properties.setShaderProgram(&m_geometry);
	m_geometry.bind();

	scene->render(properties);

	m_geometry.release();
	m_gBuffer.release();
	GL_CHECK_ERROR("DeferredRenderer::renderScene - end");
}

void DeferredRenderer::doAllShading(RenderProperties& properties, const Scene* scene) {
	GL_CHECK_ERROR("DeferredRenderer::doAllShading - begin");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_shade.bind();

	glUniform3fv(m_shade["light.color"], 1, glm::value_ptr(m_dirLight.color));
	glUniform3fv(m_shade["light.direction"], 1, glm::value_ptr(m_dirLight.direction));

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	m_gBuffer.bindTextures();

	m_fullscreenQuad.draw();

	m_shade.release();
	GL_CHECK_ERROR("DeferredRenderer::doAllShading - end");
}
