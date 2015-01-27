#include "DeferredRenderer.h"
#include "Scene.h"
#include "MinMaxHierarchy.h"

#include <thread>
#include <glm/ext.hpp>
#include <iostream>
using namespace std;

DeferredRenderer::DeferredRenderer(const DirectionalLight& light, GLuint width, GLuint height) 
	: m_fullscreenQuad(vec2(-1.0), vec2(1.0)), m_gBuffer(width, height, true), m_dirLight(light),
	m_useReferenceShadow(false)
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

		Shader passthrough(GL_VERTEX_SHADER);
		passthrough.compileFile("../shader/passthrough.vert");

		m_shade.addShader(passthrough);
		m_shade.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/shade.frag");
		m_shade.link();

		m_create_sm.addShaderFromFile(GL_VERTEX_SHADER, "../shader/create_sm.vert");
		m_create_sm.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/create_sm.frag");
		m_create_sm.link();

		m_writeSM.addShader(passthrough);
		m_writeSM.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/writeSM.frag");
		m_writeSM.link();

		m_writeImg.addShader(passthrough);
		m_writeImg.addShaderFromFile(GL_FRAGMENT_SHADER, "../shader/writeImg.frag");
		m_writeImg.link();

	} catch (ShaderException& exc) {
		cout << exc.where() << " - " << exc.what() << endl;
		std::terminate();
	} catch (FileNotFound& exc) {
		cout << exc.what() << endl;
		std::terminate();
	}

	/* Add uniforms to the shaders */
	m_geometry.addUniform("MVP");
	m_geometry.addUniform("M");
	m_geometry.addUniform("NormalMatrix");
	m_geometry.addUniform("material.shininess");
	m_geometry.addUniform("material.diffuse_color");

	m_shade.addUniform("light.color");
	m_shade.addUniform("light.direction");
	m_shade.addUniform("renderShadow");
	m_shade.addUniform("lightViewProj");

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
}

void DeferredRenderer::resize(GLuint width, GLuint height) {
	m_gBuffer.resize(width, height);

	m_visibilities->resize(width, height);
}

inline void setShadowMappingOpenGL() {
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.1f, 4.0f);
}

inline void renderSceneForSM(const Scene* scene, RenderProperties& props) {
	GL_CHECK_ERROR("DeferredRenderer::renderSceneForSM: ");
	glClear(GL_DEPTH_BUFFER_BIT);
	scene->render(props);
}

unique_ptr<ShadowMap> DeferredRenderer::renderShadowMap(const Scene* scene, uint size) {
	assert(isPowerOfTwo(size));
	glViewport(0, 0, size, size);

	/* Init Fbo to render shadow map into */
	Fbo shadowFbo(size, size, false);
	shadowFbo.bind();
	shadowFbo.setDepthTexture(GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT);
	glDrawBuffer(GL_NONE);

	mat4 lightView = m_dirLight.getViewTransform();
	mat4 lightProj = m_dirLight.getProjection();

	RenderProperties smProps(lightView, lightProj);
	smProps.setShaderProgram(&m_create_sm);
	m_create_sm.bind();

	setShadowMappingOpenGL();

	renderSceneForSM(scene, smProps);

	glDisable(GL_POLYGON_OFFSET_FILL);

	m_create_sm.release();

	GL_CHECK_ERROR("DeferredRenderer::renderShadowMap - end: ");
	return make_unique<ShadowMap>(shadowFbo.getDepthTexture());
}

/* Given a size and a number of slices, this returns the number of 2x2xnumSlices tiles */
inline uint getRestOfTiles(uint size, uint numSlices) {
	return size / (4 * numSlices);
}

void createShadowTiles(CsContainerIt shadowIt, const MinMaxHierarchy& minMax, uint x, uint y,
		uint tileNr, uint numSlices, uint totalSize) {

	vector<std::thread> shadowThreads;
	shadowThreads.reserve(numSlices);

	const uint distNextTile = totalSize / numSlices;

	for (uint tile = 0; tile < numSlices; tile += 2) {
		const uint tileOffset = tileNr * 8 + tile * distNextTile;

		auto firstIt = shadowIt + tileOffset + 2 * y + x;
		auto secondIt = firstIt + 4;

		auto create = [&minMax, numSlices](auto it, uint t) {
				*it = std::move(CompressedShadow::create(minMax, t, numSlices)); };

		shadowThreads.emplace_back(std::thread(create, firstIt, tile));
		shadowThreads.emplace_back(std::thread(create, secondIt, tile + 1));
	}

	for (auto& thread : shadowThreads)
		thread.join();
}

unique_ptr<CompressedShadow> renderWithTiles(const Scene* scene, RenderProperties& props,
		const DirectionalLight& light, Fbo& shadowFbo, uint numSlices) {

	const uint size = numSlices * numSlices * numSlices;
	CsContainer shadows(size);

	const uint restOfTiles = getRestOfTiles(size, numSlices);

	for (uint tileNr = 0; tileNr < restOfTiles; ++tileNr) {
		for (uint y = 0; y < 2; ++y) {
			for (uint x = 0; x < 2; ++x) {
				props.P = light.getSubProjection(scene->getBoundingBox(), x, y, numSlices);
	
				renderSceneForSM(scene, props);
	
				ShadowMap sm(shadowFbo.getDepthTexture());
				MinMaxHierarchy mm(sm.createImageF());

				createShadowTiles(shadows.begin(), mm, x, y, tileNr, numSlices, size);
			}
		}
	}
	return CompressedShadow::combine(shadows);
}

void DeferredRenderer::precomputeShadows(const Scene* scene, uint size) {
	GLint maxSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);

	//DEBUGGING ONLY
	//maxSize = 1024;
	//maxSize = 2048;
	//maxSize = 4096;
	//maxSize = 8192;

	if (size < maxSize) maxSize = size;

	const uint numTiles = size / maxSize;

	glViewport(0, 0, maxSize, maxSize);

	RenderProperties smProps;
	smProps.V = m_dirLight.getViewTransform();
	smProps.setShaderProgram(&m_create_sm);
	m_create_sm.bind();

	setShadowMappingOpenGL();

	// create FBO with fp depth
	Fbo shadowFbo(maxSize, maxSize, false);
	shadowFbo.bind();
	shadowFbo.setDepthTexture(GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT);
	glDrawBuffer(GL_NONE);

	if (numTiles == 1) {
		smProps.P = m_dirLight.getProjection();

		renderSceneForSM(scene, smProps);
		ShadowMap sm(shadowFbo.getDepthTexture());
		m_shadowDag = CompressedShadow::create(&sm);
	} else {
		m_shadowDag = renderWithTiles(scene, smProps, m_dirLight, shadowFbo, numTiles);
	}

	m_shadowDag->moveToGPU();
	glDisable(GL_POLYGON_OFFSET_FILL);

	m_create_sm.release();
	GL_CHECK_ERROR("DeferredRenderer::precomputeShadows - end: ");
}

void DeferredRenderer::renderQuad(const Quad& quad) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	quad.draw();
}

void DeferredRenderer::renderDepthTexture(const Texture2D* tex) {
	GL_CHECK_ERROR("DeferredRenderer::renderTexture - begin: ");
	tex->bindAt(0);

	m_writeSM.bind();
	renderQuad(m_fullscreenQuad);
	m_writeSM.release();

	GL_CHECK_ERROR("DeferredRenderer::renderTexture - end: ");
}

void DeferredRenderer::renderTexture(const Texture2D* tex) {
	GL_CHECK_ERROR("DeferredRenderer::renderTexture - begin: ");
	tex->bindAt(0);

	m_writeImg.bind();
	renderQuad(m_fullscreenQuad);
	m_writeImg.release();

	GL_CHECK_ERROR("DeferredRenderer::renderTexture - end: ");
}

void DeferredRenderer::render(RenderProperties& properties, const Scene* scene) {
	glViewport(0, 0, m_gBuffer.getWidth(), m_gBuffer.getHeight());
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	properties.setRenderingOfMaterials(true);
	renderScene(properties, scene);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
	glViewport(0, 0, m_gBuffer.getWidth(), m_gBuffer.getHeight());

	m_shade.bind();

	glUniform3fv(m_shade["light.direction"], 1, glm::value_ptr(m_dirLight.getDirection()));

	m_gBuffer.bindTextures();

	if (!m_useReferenceShadow) {
		glUniform1i(m_shade["renderShadow"], 1);

		m_shadowDag->compute(m_gBuffer.getTexture(0).get(), m_dirLight.getViewProj(), m_visibilities.get());
		m_shade.bind();
		m_visibilities->bindAt(3);
	} else {
		glUniform1i(m_shade["renderShadow"], 0);
		glUniformMatrix4fv(m_shade["lightViewProj"], 1, GL_FALSE, glm::value_ptr(m_dirLight.getViewProj()));

		m_shadowMap->bindAt(4);
	}

	renderQuad(m_fullscreenQuad);

	m_shade.release();
	GL_CHECK_ERROR("DeferredRenderer::doAllShading - end");
}
