#ifndef COMPRESSED_SHADOW_H
#define COMPRESSED_SHADOW_H

#include "cpvs.h"

class MinMaxHierarchy;
class ShadowMap;

namespace cs {
	struct AABB;
	struct Node;
};

/**
 * This entral datastructure of the CPVS represents the DAG of voxels
 * which is a compressed shadow of a light.
 *
 * The datastructure is build using a shadow map (actually it's min-max hierarchy).
 *
 * @see 'Compact Precomputed Voxelized Shadows' by E. Sintorn, V. Kaempe, O. Olsson, U. Assarsson (2014)
 * and 'High Resolution Sparse Voxel DAGs' by V. Kaempe, E. Sintorn, U. Assarsson (2013)
 */
class CompressedShadow {
public:
	enum NodeVisibility {
		SHADOW = 0,
		VISIBLE = 1,
		PARTIAL = 2
	};

private:
	/**
	 * Use the 'create' factory functions.
	 */
	CompressedShadow(const MinMaxHierarchy& minMax);

public:
	/* Constructors, destructors,... and factory functions */

	~CompressedShadow() = default;

	CompressedShadow(const CompressedShadow&) = default;
	CompressedShadow& operator=(const CompressedShadow&) = default;

	CompressedShadow(CompressedShadow&&) = default;
	CompressedShadow& operator=(CompressedShadow&&) = default;

	/**
	 * Creates a CompressedShadow from a min-max hierarchy of depth values.
	 * This will first create a Sparse Voxel Octree (SVO) from the min-max hierarchy,
	 * then merge common subtrees to create the final DAG.
	 *
	 * @see Section 6.1 
	 *
	 * @note Possible optimizations:
	 * 		- Merge three lowest level and store leaves as 4x4x4-bit grids
	 * 		- Store top of the DAG in a grid structure
	 * 		- Closed objects optimization
	 * 		- Create DAG from SVO on the GPU (as outlined in Kaempte et al. 2013)
	 */
	static unique_ptr<CompressedShadow> create(const MinMaxHierarchy& minMax);

	/**
	 * Creates a CompressedShadow from a shadow map.
	 * @note This will create a temporary min-max hierarchy.
	 */
	static unique_ptr<CompressedShadow> create(const ShadowMap& shadowMap);

public:
	/* Public methods */

	/**
	 * Traverses the sparse voxel DAG (on the CPU) for the given position
	 * in normal device coordinates, i.e. in [-1, 1]^3.
	 *
	 * @note Useful for testing purposes!
	 */
	NodeVisibility traverse(vec3 position);

	/**
	 * Given the number of levels in the octree, this calculates the resolution.
	 */
	inline static size_t getResolution(size_t numLevels) {
		return 1 << (numLevels - 1);
	}


private:
	/* Private member and helper functions */

	/**
	 * Given a point in normalized device coordinates, this calculates 
	 * the corresponding discrete coordinates within a 3-dimensional grid.
	 */
	inline static ivec3 getPathFromNDC(vec3 ndc, size_t numLevels) {
		size_t resolution = getResolution(numLevels);
		ndc += vec3(1.0f, 1.0f, 1.0f);
		ndc *= 0.5f;
		return ivec3(ndc.x * resolution, ndc.y * resolution, ndc.z * resolution);
	}

	/**
	 * During the construction of the SVO/DAG each node will have 8 pointers to it's children.
	 * This function will compress this structure 'm_dag' by removing all unnecessary pointers.
	 */
	void compressDAG();


	void constructSvoSubtree(const MinMaxHierarchy& minMax, size_t level, cs::Node* top);

private:
	/* Private class members */
	size_t m_numLevels;

	vector<uint> m_dag;
};

#endif
