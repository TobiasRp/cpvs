#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include "cpvs.h"
#include "RenderProperties.h"
#include "ShaderProgram.h"
#include "Quad.h"
#include "Fbo.h"
#include "Light.h"
#include "ShadowMap.h"
#include "CompressedShadow.h"
#include "CompressedShadowContainer.h"

class Scene;

class DeferredRenderer {
public:
	DeferredRenderer(const DirectionalLight& light, GLuint width, GLuint height);
	~DeferredRenderer() = default;

	void resize(GLuint width, GLuint height);

	void render(RenderProperties& properties, const Scene* scene);

	/**
	 * Renders a shadow map for the directional light source.
	 * @param size Must be a power of two.
	 */
	unique_ptr<ShadowMap> renderShadowMap(const Scene* scene, uint size);

	void precomputeShadows(const Scene* scene, uint size, uint pcfSize);

	void renderDepthTexture(const Texture2D* tex);

	void renderDEBUG(uint x);

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

	static void renderQuad(const Quad& quad);

	void renderScene(RenderProperties& properties, const Scene* scene);

	void doAllShading(RenderProperties& properties, const Scene* scene);

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
