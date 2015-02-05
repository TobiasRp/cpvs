#ifndef SCENE_H
#define SCENE_H

#include "cpvs.h"
#include "BoundingVolumes.h"

struct Material {
	vec3 diffuseColor;
	int shininess;

	Material() : diffuseColor(0), shininess(0) { }

	inline bool operator==(const Material& rhs) const {
		return (diffuseColor == rhs.diffuseColor) && (shininess == rhs.shininess);
	}

	inline bool operator!=(const Material& rhs) const {
		return !(*this == rhs);
	}
};

struct Mesh {
	GLuint vao;
	GLuint numFaces;
	Material material;
	AABB boundingBox;

	inline void bind() const {
		glBindVertexArray(vao);
	}

	inline void draw() const {
		glDrawElements(GL_TRIANGLES, numFaces * 3, GL_UNSIGNED_INT, 0);
	}
};

struct Scene {
	Scene() = default;

	Scene(const Scene& rhs) = delete;
	Scene& operator=(const Scene& rhs) = delete;

	Scene(Scene&& rhs) = default;
	Scene& operator=(Scene&& rhs) = default;

	inline ~Scene() {
		for (const auto& mesh : meshes)
			glDeleteVertexArrays(1, &mesh.vao);
	}

	AABB boundingBox;
	std::vector<Mesh> meshes;
};

#endif
