#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include "cpvs.h"
#include "RenderProperties.h"
#include "ShaderProgram.h"
#include "Quad.h"
#include "Fbo.h"
#include "Light.h"
#include "ShadowMap.h"
#include "PostProcess.h"
#include "CompressedShadow.h"

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
	unique_ptr<ShadowMap> renderShadowMap(const Scene* scene, int size);

	void renderTexture(shared_ptr<Texture2D> tex);

	inline DirectionalLight& getLight() {
		return m_dirLight;
	}

	inline void setPostProcess(shared_ptr<PostProcess> pp) {
		m_postProcess = pp;
	}

	inline void removePostProcess() {
		m_postProcess.reset();
	}

	inline void useShadows(bool use) {
		m_useShadowDag = use;
	}

	inline void setShadow(unique_ptr<CompressedShadow> shadowDag) {
		m_shadowDag = std::move(shadowDag);
	}

private:
	void loadShaders();
	void initFbos();

	void renderScene(RenderProperties& properties, const Scene* scene);

	void doAllShading(RenderProperties& properties, const Scene* scene);

private:
	ShaderProgram m_geometry, m_shade;
	ShaderProgram m_create_sm, m_writeImg;

	Fbo m_gBuffer;
	Fbo m_imgBuffer;

	const Quad m_fullscreenQuad;
	shared_ptr<PostProcess> m_postProcess;

	DirectionalLight m_dirLight;

	bool m_useShadowDag;
	unique_ptr<CompressedShadow> m_shadowDag;
	unique_ptr<Texture2D> m_visibilities;
};

#endif
