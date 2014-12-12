#include "DeferredRenderer.h"

#include "Scene.h"

#include <glm/ext.hpp>
#include <iostream>
using namespace std;

DeferredRenderer::DeferredRenderer(const DirectionalLight& light, int width, int height) 
	: m_fullscreenQuad(vec2(-1.0, -1.0), vec2(1.0, 1.0)), m_gBuffer(width, height, true), 
	m_imgBuffer(width, height, false), m_shadowMap(width, height, false), m_dirLight(light)
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

		m_create_sm.addShaderFromFile(GL_VERTEX_SHADER, "../shader/create_sm.vert");
		m_create_sm.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/create_sm.frag");
		m_create_sm.link();
	} catch (ShaderException& exc) {
		cout << exc.what() << endl;
		std::terminate();
	}

	/* Add uniforms to the shader */
	m_geometry.addUniform("MVP");
	m_geometry.addUniform("M");
	m_geometry.addUniform("NormalMatrix");
	m_geometry.addUniform("material.shininess");
	m_geometry.addUniform("material.diffuse_color");

	m_shade.addUniform("shadowMode");
	m_shade.addUniform("lightVP");
	m_shade.addUniform("light.color");
	m_shade.addUniform("light.direction");

	m_create_sm.addUniform("MVP");
}

void DeferredRenderer::initFbos() {
	/* init gBuffer with FP texture for position + depth, and 8-bit textures for normal and color */
	m_gBuffer.bind();
	m_gBuffer.addTexture(GL_RGBA32F);
	m_gBuffer.addTexture(GL_RGBA8);
	m_gBuffer.addTexture(GL_RGBA8);

	auto gBuffers = m_gBuffer.getColorAttachments();
	glDrawBuffers(gBuffers.size(), gBuffers.data());
	m_gBuffer.release();

	/* Image with 4x32bit FP */
	m_imgBuffer.bind();
	m_imgBuffer.addTexture(GL_RGBA32F);

	auto imgBuffers = m_imgBuffer.getColorAttachments();
	glDrawBuffers(imgBuffers.size(), imgBuffers.data());
	m_imgBuffer.release();

	/* Shadow map with 32bit fp */
	m_shadowMap.bind();
	m_shadowMap.addTexture(GL_R32F);

	auto smBuffers = m_shadowMap.getColorAttachments();
	glDrawBuffers(smBuffers.size(), smBuffers.data());
	m_shadowMap.release();
}

void DeferredRenderer::resize(int width, int height) {
	m_gBuffer.resize(width, height);
	m_imgBuffer.resize(width, height);

	if (m_postProcess.get() != nullptr)
		m_postProcess->resize(width, height);
}

void DeferredRenderer::render(RenderProperties& properties, const Scene* scene) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderScene(properties, scene);

	if (m_dirLight.shouldRenderShadowMap()) {
		renderShadowMap(scene);
	}

	doAllShading(properties, scene);

	if (m_postProcess.get() != nullptr) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		m_postProcess->render(m_gBuffer, m_imgBuffer, m_fullscreenQuad);
	}
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

void DeferredRenderer::renderShadowMap(const Scene* scene) {
	m_shadowMap.bind();
	m_shadowMap.clear();

	mat4 lightView = m_dirLight.getLightView();
	mat4 lightProj = m_dirLight.getLightProj();

	RenderProperties props(lightView, lightProj);
	props.setShaderProgram(&m_create_sm);
	m_create_sm.bind();

	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);

	scene->render(props);

	glDisable(GL_POLYGON_OFFSET_FILL);

	m_create_sm.release();
	m_shadowMap.release();
}

void DeferredRenderer::doAllShading(RenderProperties& properties, const Scene* scene) {
	GL_CHECK_ERROR("DeferredRenderer::doAllShading - begin");

	/* Either write to screen or to image for further post processing */
	if (m_postProcess.get() == nullptr) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	} else
		m_imgBuffer.bind();

	m_shade.bind();

	auto lightVP = m_dirLight.getLightProj() * m_dirLight.getLightView();
	glUniformMatrix4fv(m_shade["lightVP"], 1, GL_FALSE, glm::value_ptr(lightVP));
	glUniform1i(m_shade["shadowMode"], m_dirLight.getShadowModeID());
	glUniform3fv(m_shade["light.color"], 1, glm::value_ptr(m_dirLight.color));
	glUniform3fv(m_shade["light.direction"], 1, glm::value_ptr(m_dirLight.direction));

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	m_gBuffer.bindTextures();
	m_shadowMap.bindTextures(m_gBuffer.getNumTextures());

	m_fullscreenQuad.draw();

	m_shade.release();
	GL_CHECK_ERROR("DeferredRenderer::doAllShading - end");
}
