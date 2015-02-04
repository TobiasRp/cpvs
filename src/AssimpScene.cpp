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

template<typename T_VEC1, typename T_VEC2>
inline vec3 vecMin(const T_VEC1 a, const T_VEC2 b) {
	return vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

template<typename T_VEC1, typename T_VEC2>
inline vec3 vecMax(const T_VEC1 a, const T_VEC2 b) {
	return vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

void findBoundingBox(const aiMesh* mesh, AABB& boundingBox) {
	boundingBox.min = vec3(0.01f, 0.01f, 0.01f);
	boundingBox.max = vec3(0.01f, 0.01f, 0.01f);

	for (uint v = 0; v < mesh->mNumVertices; ++v) {
		const aiVector3D vertex = mesh->mVertices[v];
		boundingBox.min = vecMin(boundingBox.min, vertex);
		boundingBox.max = vecMax(boundingBox.max, vertex);
	}
}

void findBoundingBox(const vector<Mesh>& meshes, AABB& boundingBox) {
	boundingBox.min = vec3(0.01f, 0.01f, 0.01f);
	boundingBox.max = vec3(0.01f, 0.01f, 0.01f);
	for (const auto& mesh : meshes) {
		boundingBox.min = vecMin(boundingBox.min, mesh.boundingBox.min);
		boundingBox.max = vecMax(boundingBox.max, mesh.boundingBox.max);
	}
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

		findBoundingBox(aimesh, mesh.boundingBox);

		resScene->meshes.push_back(mesh);
	}

	findBoundingBox(resScene->meshes, resScene->boundingBox);
}
