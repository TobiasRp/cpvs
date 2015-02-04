#ifndef ASSIMP_RENDERER_H
#define ASSIMP_RENDERER_H

#include "cpvs.h"
#include "Scene.h"
#include <assimp/Importer.hpp>

class AssimpScene {
protected:
	enum AttributeLog {
		vPosLoc = 0,
		vNormalLoc,
		vTexCoordLoc,
	};

public:
	AssimpScene() = delete;	
	~AssimpScene() = delete;

	static unique_ptr<Scene> loadScene(const string file);

protected:
	static void genVAOsAndUniformBuffer(const aiScene* aiscene, Scene* resScene);
};

#endif
