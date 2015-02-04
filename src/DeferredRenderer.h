#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include "cpvs.h"
#include "ShaderProgram.h"
#include "Quad.h"
#include "Fbo.h"
#include "Light.h"
#include "ShadowMap.h"
#include "CompressedShadow.h"
#include "CompressedShadowContainer.h"
#include "Camera.h"

class Scene;

class DeferredRenderer {
public:
	DeferredRenderer(const DirectionalLight& light, GLuint width, GLuint height);
	~DeferredRenderer() = default;

	void resize(GLuint width, GLuint height);

	void render(Camera* cam, const Scene* scene);

	/**
	 * Renders a shadow map for the directional light source.
	 * @param size Must be a power of two.
	 */
	unique_ptr<ShadowMap> renderShadowMap(const Scene* scene, uint size);

	void precomputeShadows(const Scene* scene, uint size, uint pcfSize);

	void renderDepthTexture(const Texture2D* tex);

	void renderTexture(const Texture2D* tex);

	inline DirectionalLight& getLight() {
		return m_dirLight;
	}

	inline void useReferenceShadows(bool use) {
		m_useReferenceShadow = use;
	}

	inline void setShadow(shared_ptr<Texture2D> shadowMap) {
		m_shadowMap = std::move(shadowMap);
	}

private:
	void loadShaders();
	void initFbos();

	void renderSceneForSM(const Scene* scene, const mat4& P, const mat4& V);

	unique_ptr<CompressedShadowContainer> renderWithTiles(const Scene* scene, const mat4& V,
		const DirectionalLight& light, Fbo& shadowFbo, uint numSlices);

	static void renderQuad(const Quad& quad);

	void renderScene(Camera* cam, const Scene* scene);

	void doAllShading();

private:
	ShaderProgram m_geometry, m_shade;
	ShaderProgram m_create_sm, m_writeSM, m_writeImg;

	Fbo m_gBuffer;

	const Quad m_fullscreenQuad;

	DirectionalLight m_dirLight;

	bool m_useReferenceShadow;
	unique_ptr<CompressedShadowContainer> m_precomputedShadow;
	unique_ptr<Texture2D> m_visibilities;

	shared_ptr<Texture2D> m_shadowMap;
};

#endif
