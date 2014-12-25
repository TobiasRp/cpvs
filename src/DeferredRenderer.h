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
#include "Buffer.h"

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

	/**
	 * Specified the precomputed shadow to use (if shadows are enabled)
	 */
	inline void setShadowDAG(shared_ptr<SSBO> dag) {
		m_shadowDag = dag;

		const GLuint width = m_gBuffer.getWidth();
		const GLuint height = m_gBuffer.getHeight();
		m_visibilities = std::make_unique<Texture2D>(width, height, GL_R8, GL_RED, GL_UNSIGNED_BYTE);
	}

	inline void useShadows(bool use) {
		m_useShadowDag = use;
	}

private:
	void loadShaders();
	void initFbos();

	void renderScene(RenderProperties& properties, const Scene* scene);

	void doAllShading(RenderProperties& properties, const Scene* scene);

	void computeShadow();

private:
	ShaderProgram m_geometry, m_shade;
	ShaderProgram m_create_sm, m_writeImg;
	ShaderProgram m_traverseCS;

	Fbo m_gBuffer;
	Fbo m_imgBuffer;

	const Quad m_fullscreenQuad;
	shared_ptr<PostProcess> m_postProcess;

	DirectionalLight m_dirLight;

	bool m_useShadowDag;
	shared_ptr<SSBO> m_shadowDag;
	unique_ptr<Texture2D> m_visibilities;
};

#endif
