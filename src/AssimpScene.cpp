#include "AssimpScene.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

//#include <IL/il.h>

#include <iostream>
#include <fstream>

using namespace std;
using namespace Assimp;

AssimpScene::AssimpScene(const std::string file) {
	// maybe test if file exists?
	ifstream is(file);
	if (!is.is_open()) {
		throw FileNotFound("Specified file not found");
	}
	
	auto scene = m_importer.ReadFile(file, aiProcessPreset_TargetRealtime_Quality);
	if (!scene) {
		throw LoadFileException("Assimp couldn't load file");
	}

	cout << "Assimp has read the file" << endl;

	genVAOsAndUniformBuffer(scene);

	cout << "Finished moving the data to the GPU" << endl;
}

void AssimpScene::genVAOsAndUniformBuffer(const aiScene *scene) {
	GLuint buffer;
	AssimpMesh mesh;
	m_meshes.reserve(scene->mNumMeshes);

	for (unsigned n = 0; n < scene->mNumMeshes; ++n) {
		aiMesh *aimesh = scene->mMeshes[n];

		auto mat = scene->mMaterials[aimesh->mMaterialIndex];
		aiColor3D diffColor;
		mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffColor);
		mesh.color = vec3(diffColor.r, diffColor.g, diffColor.b);

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
	program->bind();
	program->setUniformMatrix4fv("MVP", glm::value_ptr(MVP));
	program->setUniformMatrix3fv("NormalMatrix", glm::value_ptr(props.getNormalMatrix()));

	GL_CHECK_ERROR("recursiveRender: ");

	for (unsigned n = 0; n < node->mNumMeshes; ++n) {
		auto index = node->mMeshes[n];
		glBindVertexArray(m_meshes[index].vao);

		/* set diffuse color */
		program->setUniform3fv("diffuse_color", glm::value_ptr(m_meshes[index].color));

		glDrawElements(GL_TRIANGLES, m_meshes[index].numFaces * 3, GL_UNSIGNED_INT, 0);
	}

	for (unsigned n = 0; n < node->mNumChildren; ++n) {
		recursiveRender(props, sc, node->mChildren[n]);
	}

	GL_CHECK_ERROR("recursiveRender (2): ");

	stack->pop();
}

void AssimpScene::render(RenderProperties &props) const noexcept {
	auto scene = m_importer.GetScene();

	recursiveRender(props, scene, scene->mRootNode);
}
