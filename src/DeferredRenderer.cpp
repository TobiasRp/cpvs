#include "DeferredRenderer.h"

#include "Scene.h"

#include <glm/ext.hpp>
#include <iostream>
using namespace std;

DeferredRenderer::DeferredRenderer(const DirectionalLight& light, int width, int height) 
	: m_fullscreenQuad(vec2(-1.0, -1.0), vec2(1.0, 1.0)),
	m_gBuffer(width, height, true), m_imgBuffer(width, height, false),
	m_dirLight(light)
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

	m_shade.addUniform("light.color");
	m_shade.addUniform("light.direction");

	m_create_sm.addUniform("MVP");
}

void DeferredRenderer::initFbos() {
	/* init gBuffer with FP texture for position + depth, and 8-bit textures for normal and color */
	m_gBuffer.bind();
	GL_CHECK_ERROR("bound gBuffer: ");
	m_gBuffer.addTexture(GL_RGBA32F, GL_RGBA, GL_FLOAT);
	m_gBuffer.addTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	m_gBuffer.addTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);

	auto gBuffers = m_gBuffer.getColorAttachments();
	glDrawBuffers(gBuffers.size(), gBuffers.data());
	m_gBuffer.release();

	/* Image with 4x32bit FP */
	m_imgBuffer.bind();
	m_imgBuffer.addTexture(GL_RGBA32F, GL_RGBA, GL_FLOAT);

	auto imgBuffers = m_imgBuffer.getColorAttachments();
	glDrawBuffers(imgBuffers.size(), imgBuffers.data());
	m_imgBuffer.release();
}

void DeferredRenderer::resize(int width, int height) {
	m_gBuffer.resize(width, height);
	m_imgBuffer.resize(width, height);

	if (m_postProcess.get() != nullptr)
		m_postProcess->resize(width, height);
}

unique_ptr<ShadowMap> DeferredRenderer::renderShadowMap(const Scene* scene, int width, int height) {
	/* Init Fbo to render shadow map into */
	Fbo shadowFbo(width, height, false);
	shadowFbo.bind();
	shadowFbo.addTexture(GL_R32F, GL_RED, GL_FLOAT);
	glDrawBuffers(1, shadowFbo.getColorAttachments().data());

	shadowFbo.clear();

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

	return make_unique<ShadowMap>(width, height, shadowFbo.getTexture(0));
}


void DeferredRenderer::render(RenderProperties& properties, const Scene* scene) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderScene(properties, scene);

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

void DeferredRenderer::doAllShading(RenderProperties& properties, const Scene* scene) {
	GL_CHECK_ERROR("DeferredRenderer::doAllShading - begin");

	/* Either write to screen or to image for further post processing */
	if (m_postProcess.get() == nullptr) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	} else
		m_imgBuffer.bind();

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
