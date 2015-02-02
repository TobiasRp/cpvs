#include "AssimpScene.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

#include <iostream>
#include <fstream>

using namespace std;
using namespace Assimp;

AssimpScene::AssimpScene(const std::string file) {
	ifstream is(file);
	if (!is.is_open()) {
		throw FileNotFound("Specified file not found");
	}
	
	auto scene = m_importer.ReadFile(file, aiProcessPreset_TargetRealtime_Fast);
	if (!scene) {
		throw LoadFileException("Assimp couldn't load file");
	}

	genVAOsAndUniformBuffer(scene);
}

void setMaterial(AssimpMesh& mesh, const aiMaterial* mat) {
	aiColor3D diffColor;
	mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffColor);
	mesh.color = vec3(diffColor.r, diffColor.g, diffColor.b);

	mat->Get(AI_MATKEY_SHININESS, mesh.shininess);
}

void findBoundingBoxForNode(const aiScene* scene, const aiNode* node, aiVector3D* min, aiVector3D* max) {
	for (uint m = 0; m < node->mNumMeshes; ++m) {
		const aiMesh* mesh = scene->mMeshes[node->mMeshes[m]];
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
		findBoundingBoxForNode(scene, node->mChildren[child], min, max);
	}
}

AABB findBoundingBox(const aiScene* scene, const aiNode* root) {
	aiVector3D min, max;
	min.x = min.y = min.z = 0.1f;
	max.x = max.y = max.z = 0.1f;
	findBoundingBoxForNode(scene, root, &min, &max);

	AABB result;
	result.min = vec3(min.x, min.y, min.z);
	result.max = vec3(max.x, max.y, max.z);
	return result;
}

void AssimpScene::genVAOsAndUniformBuffer(const aiScene *scene) {
	GLuint buffer;
	AssimpMesh mesh;
	m_meshes.reserve(scene->mNumMeshes);

	for (unsigned n = 0; n < scene->mNumMeshes; ++n) {
		aiMesh *aimesh = scene->mMeshes[n];

		auto mat = scene->mMaterials[aimesh->mMaterialIndex];
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

		m_meshes.push_back(mesh);
	}

	m_boundingBox = findBoundingBox(scene, scene->mRootNode);
}

void AssimpScene::recursiveRender(RenderProperties &props, const aiScene *sc, const aiNode *node) const noexcept {
	aiMatrix4x4 mat = node->mTransformation;
	mat.Transpose(); // OpenGL has column major matrices

	auto stack = props.getModelStack();
	auto glmMat = glm::make_mat4((float*) &mat);
	stack->push(glmMat);

	GL_CHECK_ERROR("recursiveRender (1): ");

	mat4 MVP = props.getMVP();
	auto program = props.getShaderProgram();

	if (program->hasUniform("MVP")) {
		auto mvp = program->getUniformLoc("MVP");
		glUniformMatrix4fv(mvp, 1, false, glm::value_ptr(MVP));
	}

	if (program->hasUniform("M")) {
		auto M = props.getM();
		auto mLoc = program->getUniformLoc("M");
		glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(M));
	}

	if (program->hasUniform("NormalMatrix")) {
		auto normalMat = props.getNormalMatrix();
		auto nm = program->getUniformLoc("NormalMatrix");
		glUniformMatrix3fv(nm, 1, false, glm::value_ptr(normalMat));
	}

	for (unsigned n = 0; n < node->mNumMeshes; ++n) {
		auto index = node->mMeshes[n];
		auto mesh = m_meshes[index];
		glBindVertexArray(m_meshes[index].vao);

		if (props.renderMaterials()) {
			auto diffCol = program->getUniformLoc("material.diffuse_color");
			glUniform3fv(diffCol, 1, glm::value_ptr(mesh.color));

			auto sh = program->getUniformLoc("material.shininess");
			glUniform1i(sh, mesh.shininess);
		}
		glDrawElements(GL_TRIANGLES, mesh.numFaces * 3, GL_UNSIGNED_INT, 0);
		GL_CHECK_ERROR("recursiveRender after draw: ");
	}

	for (unsigned n = 0; n < node->mNumChildren; ++n) {
		GL_CHECK_ERROR("recursiveRender (before calling recursively): ");
		recursiveRender(props, sc, node->mChildren[n]);
	}

	GL_CHECK_ERROR("recursiveRender (2): ");

	stack->pop();
}

void AssimpScene::render(RenderProperties &props) const noexcept {
	GL_CHECK_ERROR("AssimpScene::render - begin : ");
	auto scene = m_importer.GetScene();

	auto program = props.getShaderProgram();
	if (program->hasUniform("V")) {
		auto loc = program->getUniformLoc("V");
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(props.V));
	}

	if (program->hasUniform("P")) {
		auto loc = program->getUniformLoc("P");
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(props.P));
	}

	recursiveRender(props, scene, scene->mRootNode);
}
