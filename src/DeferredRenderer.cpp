#include "DeferredRenderer.h"

#include "Scene.h"

#include <glm/ext.hpp>
#include <iostream>
using namespace std;

DeferredRenderer::DeferredRenderer(const DirectionalLight& light, GLuint width, GLuint height) 
	: m_fullscreenQuad(vec2(-1.0, -1.0), vec2(1.0, 1.0)),
	m_gBuffer(width, height, true), m_imgBuffer(width, height, false),
	m_dirLight(light)
{
	loadShaders();
	initFbos();

	m_visibilities = make_unique<Texture2D>(width, height, GL_R8, GL_RED, GL_UNSIGNED_BYTE);
}

void DeferredRenderer::loadShaders() {
	try {
		m_geometry.addShaderFromFile(GL_VERTEX_SHADER, "../shader/geometry.vert");
		m_geometry.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/geometry.frag");
		m_geometry.link();

		m_shade.addShaderFromFile(GL_VERTEX_SHADER, "../shader/passthrough.vert");
		m_shade.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/shade.frag");
		m_shade.link();

		m_create_sm.addShaderFromFile(GL_VERTEX_SHADER, "../shader/create_sm.vert");
		m_create_sm.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/create_sm.frag");
		m_create_sm.link();

		m_writeImg.addShaderFromFile(GL_VERTEX_SHADER, "../shader/passthrough.vert");
		m_writeImg.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/writeSM.frag");
		m_writeImg.link();
	} catch (ShaderException& exc) {
		cout << exc.where() << " - " << exc.what() << endl;
		std::terminate();
	} catch (FileNotFound& exc) {
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
	m_shade.addUniform("renderShadow");

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
	m_imgBuffer.getTexture(0)->setMinMagFiltering(GL_LINEAR, GL_LINEAR);

	auto imgBuffers = m_imgBuffer.getColorAttachments();
	glDrawBuffers(imgBuffers.size(), imgBuffers.data());
	m_imgBuffer.release();
}

void DeferredRenderer::resize(GLuint width, GLuint height) {
	m_gBuffer.resize(width, height);
	m_imgBuffer.resize(width, height);

	if (m_postProcess.get() != nullptr)
		m_postProcess->resize(width, height);

	m_visibilities->resize(width, height);
}

unique_ptr<ShadowMap> DeferredRenderer::renderShadowMap(const Scene* scene, int size) {
	assert(isPowerOfTwo(size));

	/* Init Fbo to render shadow map into */
	Fbo shadowFbo(size, size, false);
	shadowFbo.bind();
	shadowFbo.setDepthTexture(GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT);
	glDrawBuffer(GL_NONE);

	glViewport(0, 0, size, size);

	mat4 lightView = m_dirLight.getLightView();
	mat4 lightProj = m_dirLight.getLightProj();

	RenderProperties smProps(lightView, lightProj);
	smProps.setShaderProgram(&m_create_sm);
	m_create_sm.bind();

	glClearDepth(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.1f, 4.0f);

	scene->render(smProps);

	glDisable(GL_POLYGON_OFFSET_FILL);

	m_create_sm.release();

	return make_unique<ShadowMap>(shadowFbo.getDepthTexture());
}

void DeferredRenderer::renderTexture(shared_ptr<Texture2D> tex) {
	GL_CHECK_ERROR("DeferredRenderer::renderTexture - begin: ");
	tex->bindAt(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_writeImg.bind();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	m_fullscreenQuad.draw();

	m_writeImg.release();

	GL_CHECK_ERROR("DeferredRenderer::renderTexture - end: ");
}

void DeferredRenderer::render(RenderProperties& properties, const Scene* scene) {
	glViewport(0, 0, m_gBuffer.getWidth(), m_gBuffer.getHeight());
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	properties.setRenderingOfMaterials(true);
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

	if (m_useShadowDag) {
		glUniform1i(m_shade["renderShadow"], 1);

		m_shadowDag->compute(m_gBuffer.getTexture(0).get(), m_dirLight.getLightViewProj(), m_visibilities.get());
		m_shade.bind();
		m_visibilities->bindAt(3);
	} else {
		glUniform1i(m_shade["renderShadow"], 0);
	}

	m_fullscreenQuad.draw();

	m_shade.release();
	GL_CHECK_ERROR("DeferredRenderer::doAllShading - end");
}
