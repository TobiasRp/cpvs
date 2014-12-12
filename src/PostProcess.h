#ifndef POST_PROCESS_H
#define POST_PROCESS_H

#include "cpvs.h"

class Fbo;
class Quad;

class PostProcess {
public:
	virtual ~PostProcess() { }

	/**
	 * Initializes the post process.
	 */
    virtual void initialize(int width, int height) = 0;

	/**
	 * Call this every time the window size changes.
	 */
	virtual void resize(int width, int height) = 0;

	/**
	 * Renders the post process by drawing the given quad.
	 * @param gBuffer The G-Buffer of the scene.
	 * @param inBuffer The image/buffer to process
	 *
	 * @note Make sure the right output buffer (or backbuffer) is bound.
	 */
	virtual void render(const Fbo& gBuffer, Fbo& inBuffer, const Quad& screen) = 0;


	/**
	 * Creates a new FXAA anti-aliasing post-process
	 */
	static unique_ptr<PostProcess> createFXAA(int width, int height);
};

#endif
