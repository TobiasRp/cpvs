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

class Scene;

class DeferredRenderer {
public:
	DeferredRenderer(const DirectionalLight& light, int width, int height);
	~DeferredRenderer() = default;

	void resize(int width, int height);

	void render(RenderProperties& properties, const Scene* scene);

	const DirectionalLight& getLight() {
		return m_dirLight;
	}

	unique_ptr<ShadowMap> renderShadowMap(const Scene* scene, int width, int height);

	void setPostProcess(unique_ptr<PostProcess>&& pp) {
		m_postProcess = std::move(pp);
	}

	void removePostProcess() {
		m_postProcess.reset();
	}

private:
	void loadShaders();
	void initFbos();

	void renderScene(RenderProperties& properties, const Scene* scene);

	void doAllShading(RenderProperties& properties, const Scene* scene);

private:
	ShaderProgram m_geometry, m_shade, m_create_sm;
	Fbo m_gBuffer;
	Fbo m_imgBuffer;

	const Quad m_fullscreenQuad;
	unique_ptr<PostProcess> m_postProcess;

	const DirectionalLight m_dirLight;
};

#endif
