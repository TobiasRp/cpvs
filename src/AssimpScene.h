#ifndef ASSIMP_RENDERER_H
#define ASSIMP_RENDERER_H

#include "cpvs.h"
#include "RenderProperties.h"
#include <assimp/Importer.hpp>
#include <unordered_map>

class aiNode;

struct AssimpMesh {
	GLuint vao;
	GLuint numFaces;
	vec3 color;
};


class AssimpScene {
protected:
	enum AttributeLog {
		vPosLoc = 0,
		vNormalLoc,
		vTexCoordLoc,
	};

public:
	AssimpScene(const string file);	
	~AssimpScene() { }

	void render(RenderProperties &props);

protected:
	void genVAOsAndUniformBuffer(const aiScene *scene);

	void recursiveRender(RenderProperties &props, const aiScene *scene, const aiNode *node);

private:
	Assimp::Importer m_importer;
	std::vector<AssimpMesh> m_meshes;
};

#endif
