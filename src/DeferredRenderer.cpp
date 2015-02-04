#include "DeferredRenderer.h"
#include "Scene.h"
#include "MinMaxHierarchy.h"
#include "CompressedShadowContainer.h"

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

	m_create_sm.addUniform("M");
	m_create_sm.addUniform("V");
	m_create_sm.addUniform("P");
	m_create_sm.addUniform("znear");
	m_create_sm.addUniform("zfar");

	m_writeSM.addUniform("znear");
	m_writeSM.addUniform("zfar");
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

inline void setShadowMappingState() {
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	glPolygonOffset(1.1f, 4.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);
}

inline void setNearAndFarPlane(ShaderProgram& createSM, const DirectionalLight& light) {
	glUniform1f(createSM["znear"], light.getNearPlane());
	glUniform1f(createSM["zfar"], light.getFarPlane());
}

void DeferredRenderer::renderSceneForSM(const Scene* scene, const mat4& P, const mat4& V) {
	GL_CHECK_ERROR("DeferredRenderer::renderSceneForSM: ");
	glClear(GL_DEPTH_BUFFER_BIT);

	// Assumes that m_create_sm program is bound
	glUniformMatrix4fv(m_create_sm["V"], 1, GL_FALSE, glm::value_ptr(V));
	glUniformMatrix4fv(m_create_sm["P"], 1, GL_FALSE, glm::value_ptr(P));

	for (const auto& mesh : scene->meshes) {
		mesh.bind();
		mesh.draw();
	}
}

unique_ptr<ShadowMap> DeferredRenderer::renderShadowMap(const Scene* scene, uint size) {
	assert(isPowerOfTwo(size));
	glViewport(0, 0, size, size);

	/* Init Fbo to render shadow map into */
	Fbo shadowFbo(size, size, false);
	shadowFbo.bind();
	shadowFbo.setDepthTexture(GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT);
	glDrawBuffer(GL_NONE);

	m_create_sm.bind();

	setNearAndFarPlane(m_create_sm, m_dirLight);
	setShadowMappingState();

	mat4 lightView = m_dirLight.getViewTransform();
	mat4 lightProj = m_dirLight.getProjection();
	renderSceneForSM(scene, lightProj, lightView);

	glDisable(GL_POLYGON_OFFSET_FILL);
	m_create_sm.release();

	GL_CHECK_ERROR("DeferredRenderer::renderShadowMap - end: ");
	return make_unique<ShadowMap>(shadowFbo.getDepthTexture());
}

void createShadowTiles(CompressedShadowContainer* shadows, const MinMaxHierarchy& minMax,
		uint x, uint y, uint numSlices) {

	vector<std::thread> shadowThreads;
	shadowThreads.reserve(numSlices);

	for (uint tile = 0; tile < numSlices; ++tile) {
		shadowThreads.emplace_back(std::thread([shadows, &minMax, numSlices, x, y, tile]() {
				shadows->set(CompressedShadow::create(minMax, tile, numSlices), x, y, tile); }));
	}

	for (auto& thread : shadowThreads)
		thread.join();
}

unique_ptr<CompressedShadowContainer> DeferredRenderer::renderWithTiles(const Scene* scene, const mat4& V,
		const DirectionalLight& light, Fbo& shadowFbo, uint numSlices) {

	auto shadows = make_unique<CompressedShadowContainer>(numSlices);

	for (uint y = 0; y < numSlices; ++y) {
		for (uint x = 0; x < numSlices; ++x) {
			const auto P = light.getSubProjection(scene->boundingBox, x, y, numSlices);

			renderSceneForSM(scene, P, V);

			ShadowMap sm(shadowFbo.getDepthTexture());
			MinMaxHierarchy mm(sm.createImageF());

			createShadowTiles(shadows.get(), mm, x, y, numSlices);
		}
#ifdef PRINT_PROGRESS
		cout << ((y + 1) / static_cast<float>(numSlices)) * 100 << "% ";
		cout.flush();
#endif
	}
	return shadows;
}

inline uint getTileSize(uint size) {
	GLint maxSize = 8192;

	/* My GPU has a maximum texture size of 16K which could be useful for tiling
	 * very high resolutions (> 128K), but otherwise seems to slow down performance for 
	 * lower resolutions. As a heuristic 8K seems fine. */
	//glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);

	if (size < maxSize) maxSize = size;

	return maxSize;
}

void DeferredRenderer::precomputeShadows(const Scene* scene, uint size, uint pcfSize) {
	const uint tileSize = getTileSize(size);
	const uint numTiles = size / tileSize;

	glViewport(0, 0, tileSize, tileSize);

	const auto V = m_dirLight.getViewTransform();
	m_create_sm.bind();

	setNearAndFarPlane(m_create_sm, m_dirLight);
	setShadowMappingState();

	// create FBO with floating point depth
	Fbo shadowFbo(tileSize, tileSize, false);
	shadowFbo.bind();
	shadowFbo.setDepthTexture(GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT);
	glDrawBuffer(GL_NONE);

	if (numTiles == 1) {
		const auto P = m_dirLight.getProjection();
		renderSceneForSM(scene, P, V);

		ShadowMap sm(shadowFbo.getDepthTexture());
		m_precomputedShadow = make_unique<CompressedShadowContainer>(CompressedShadow::create(&sm));
	} else {
		m_precomputedShadow = renderWithTiles(scene, V, m_dirLight, shadowFbo, numTiles);
	}
	m_precomputedShadow->setFilterSize(pcfSize);
	m_precomputedShadow->moveToGPU();

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
	setNearAndFarPlane(m_writeSM, m_dirLight);

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

void DeferredRenderer::render(Camera* cam, const Scene* scene) {
	glViewport(0, 0, m_gBuffer.getWidth(), m_gBuffer.getHeight());
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderScene(cam, scene);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	doAllShading();
}

void DeferredRenderer::renderScene(Camera* cam, const Scene* scene) {
	GL_CHECK_ERROR("DeferredRenderer::renderScene - begin: ");
	m_gBuffer.bind();
	m_gBuffer.clear();

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	
	m_geometry.bind();

	const auto MVP = cam->getProjection() * cam->getView();
	glUniformMatrix4fv(m_geometry["MVP"], 1, false, glm::value_ptr(MVP));

	const auto identity = mat4(1.0f); // model matrix M is identity
	glUniformMatrix4fv(m_geometry["M"], 1, false, glm::value_ptr(identity));

	const mat3 normalMat = mat3(1.0f); // if M is not identity use: inverse(transpose(M));
	glUniformMatrix3fv(m_geometry["NormalMatrix"], 1, false, glm::value_ptr(normalMat));

	for (const auto& mesh : scene->meshes) {
		mesh.bind();
		glUniform3fv(m_geometry["material.diffuse_color"], 1, glm::value_ptr(mesh.material.diffuseColor));
		glUniform1i(m_geometry["material.shininess"], mesh.material.shininess);
		mesh.draw();
	}

	m_geometry.release();
	m_gBuffer.release();
	GL_CHECK_ERROR("DeferredRenderer::renderScene - end: ");
}

void DeferredRenderer::doAllShading() {
	GL_CHECK_ERROR("DeferredRenderer::doAllShading - begin: ");
	glViewport(0, 0, m_gBuffer.getWidth(), m_gBuffer.getHeight());

	m_shade.bind();

	glUniform3fv(m_shade["light.direction"], 1, glm::value_ptr(m_dirLight.getDirection()));

	m_gBuffer.bindTextures();

	if (!m_useReferenceShadow) {
		glUniform1i(m_shade["renderShadow"], 1);

		m_precomputedShadow->evaluate(m_gBuffer.getTexture(0).get(), m_dirLight.getViewProj(), m_visibilities.get());
		m_shade.bind();
		m_visibilities->bindAt(3);
	} else {
		glUniform1i(m_shade["renderShadow"], 0);
		glUniformMatrix4fv(m_shade["lightViewProj"], 1, GL_FALSE, glm::value_ptr(m_dirLight.getViewProj()));

		m_shadowMap->bindAt(4);
	}

	renderQuad(m_fullscreenQuad);

	m_shade.release();
	GL_CHECK_ERROR("DeferredRenderer::doAllShading - end: ");
}
