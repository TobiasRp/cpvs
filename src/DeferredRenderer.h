#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include "cpvs.h"
#include "RenderProperties.h"
#include "ShaderProgram.h"
#include "Quad.h"
#include "Fbo.h"

class Scene;

class DeferredRenderer {
public:
	DeferredRenderer(int width, int height);
	~DeferredRenderer() = default;

	void resize(int width, int height);

	void render(RenderProperties& properties, const Scene* scene);

private:
	void loadShaders();
	void initFbos();

	void renderScene(RenderProperties& properties, const Scene* scene);

	void doAllShading(RenderProperties& properties, const Scene* scene);

private:
	ShaderProgram m_createDS, m_useDS;
	Fbo m_gBuffer;
	Quad m_fullscreenQuad;
};

#endif
