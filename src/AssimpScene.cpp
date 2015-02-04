#include "AssimpScene.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

#include <iostream>
#include <fstream>

using namespace std;
using namespace Assimp;

unique_ptr<Scene> AssimpScene::loadScene(const string file) {
	ifstream is(file);
	if (!is.is_open()) {
		throw FileNotFound("Specified file not found");
	}

	Assimp::Importer importer;
	auto assimpScene = importer.ReadFile(file, aiProcessPreset_TargetRealtime_Fast);
	if (!assimpScene) {
		throw LoadFileException("Assimp couldn't load file");
	}

	auto scene = make_unique<Scene>();
	genVAOsAndUniformBuffer(assimpScene, scene.get());

	return std::move(scene);
}

void setMaterial(Mesh& mesh, const aiMaterial* mat) {
	aiColor3D diffColor;
	mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffColor);
	mesh.material.diffuseColor = vec3(diffColor.r, diffColor.g, diffColor.b);

	mat->Get(AI_MATKEY_SHININESS, mesh.material.shininess);
}

void findBoundingBoxForNode(const aiScene* aiscene, const aiNode* node, aiVector3D* min, aiVector3D* max) {
	for (uint m = 0; m < node->mNumMeshes; ++m) {
		const aiMesh* mesh = aiscene->mMeshes[node->mMeshes[m]];
		for (uint v = 0; v < mesh->mNumVertices; ++v) {
			aiVector3D vertex = mesh->mVertices[v];

			min->x = std::min(min->x, vertex.x);
			min->y = std::min(min->y, vertex.y);
			min->z = std::min(min->z, vertex.z);

			max->x = std::max(max->x, vertex.x);
			max->y = std::max(max->y, vertex.y);
			max->z = std::max(max->z, vertex.z);
		}
	}
	for (uint child = 0; child < node->mNumChildren; ++child) {
		findBoundingBoxForNode(aiscene, node->mChildren[child], min, max);
	}
}

AABB findBoundingBox(const aiScene* aiscene, const aiNode* root) {
	aiVector3D min, max;
	min.x = min.y = min.z = 0.1f;
	max.x = max.y = max.z = 0.1f;
	findBoundingBoxForNode(aiscene, root, &min, &max);

	AABB result;
	result.min = vec3(min.x, min.y, min.z);
	result.max = vec3(max.x, max.y, max.z);
	return result;
}

void AssimpScene::genVAOsAndUniformBuffer(const aiScene* aiscene, Scene* resScene) {
	GLuint buffer;
	Mesh mesh;
	resScene->meshes.reserve(aiscene->mNumMeshes);

	for (unsigned n = 0; n < aiscene->mNumMeshes; ++n) {
		aiMesh *aimesh = aiscene->mMeshes[n];

		auto mat = aiscene->mMaterials[aimesh->mMaterialIndex];
		setMaterial(mesh, mat);

		glGenVertexArrays(1, &(mesh.vao));
		glBindVertexArray(mesh.vao);

		// Copy assimp faces to linear array
		uint *faces = new uint[aimesh->mNumFaces * 3];

		unsigned faceIndex = 0;
		for (unsigned i = 0; i < aimesh->mNumFaces; ++i) {
			auto face = &aimesh->mFaces[i];
			memcpy(&faces[faceIndex], face->mIndices, 3 * sizeof(unsigned));
			faceIndex += 3;
		}
		mesh.numFaces = aimesh->mNumFaces;

		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * aimesh->mNumFaces * 3,
				faces, GL_STATIC_DRAW);

		// Don't forget to delete dynamically allocated memory
		delete[] faces;

		if (aimesh->HasPositions()) {
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * aimesh->mNumVertices,
					aimesh->mVertices, GL_STATIC_DRAW);

			glEnableVertexAttribArray(vPosLoc);
			glVertexAttribPointer(vPosLoc, 3, GL_FLOAT, 0, 0, 0);
		}

		if (aimesh->HasNormals()) {
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * aimesh->mNumVertices,
					aimesh->mNormals, GL_STATIC_DRAW);

			glEnableVertexAttribArray(vNormalLoc);
			glVertexAttribPointer(vNormalLoc, 3, GL_FLOAT, 0, 0, 0);
		}

		//TODO texcoords
		glBindVertexArray(0);

		resScene->meshes.push_back(mesh);
	}

	resScene->boundingBox = findBoundingBox(aiscene, aiscene->mRootNode);
}
