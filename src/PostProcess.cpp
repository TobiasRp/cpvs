#include "PostProcess.h"

#include "fxaa.h"

unique_ptr<PostProcess> PostProcess::createFXAA(int width, int height) {
	auto pp = std::make_unique<FXAA>();
	pp->initialize(width, height);
	return std::move(pp);
}
