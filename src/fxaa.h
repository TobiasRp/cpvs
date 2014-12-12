#ifndef FXAA_H
#define FXAA_H

#include "cpvs.h"
#include "PostProcess.h"
#include "ShaderProgram.h"

class FXAA : public PostProcess
{
public:
    FXAA() = default;

    virtual void initialize(int width, int height) override;

    virtual void resize(int width, int height) override;

    virtual void render(const Fbo& gBuffer, Fbo& inBuffer, const Quad& fullScreen) override;

    inline void showDefault() {
        m_prog.bind();
		glUniform1i(m_prog["renderMode"], 0);
        m_prog.release();
    }

    inline void showEdges() {
        m_prog.bind();
		glUniform1i(m_prog["renderMode"], 1);
        m_prog.release();
    }

private:
    ShaderProgram m_prog;
};

#endif // FXAA_H
