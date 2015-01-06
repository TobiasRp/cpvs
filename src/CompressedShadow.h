#ifndef COMPRESSED_SHADOW_H
#define COMPRESSED_SHADOW_H

#include "cpvs.h"
#include "Buffer.h"
#include "ShaderProgram.h"
#include "Texture.h"

class MinMaxHierarchy;
class ShadowMap;

/**
 * This central datastructure of the CPVS represents the DAG of voxels
 * which is a compressed shadow of a light.
 *
 * The datastructure is built using a shadow map (actually it's min-max hierarchy).
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

	/**
	 * Traverses the sparse voxel DAG (on the CPU) for the given position
	 * in normal device coordinates, i.e. in [-1, 1]^3.
	 *
	 * @note Useful for testing purposes!
	 */
	NodeVisibility traverse(const vec3 position);
	
	/**
	 * Calculates the visibility/shadow of every world-space position in the given texture.
	 * The result is a 2-dimensional texture of visibility values.
	 */
	void compute(const Texture2D* positionsWS, const mat4& lightViewProj, Texture2D* visibilities);

	/**
	 * Frees all dynamically allocated memory for the DAG on the cpu.
	 * All operations evaluating the shadow on the GPU are still valid, but the DAG can't be traversed
	 * on the CPU or otherwise changed.
	 */
	inline void freeDagOnCPU() {
		// Use the 'swap trick' to free all dynamic memory
		std::vector<uint> tmp;
		m_dag.swap(tmp);
	}

private:
	/* Private member and helper functions */

	/**
	 * Initialize all shader and kernels
	 */
	void initShaderAndKernels();

	/**
	 * Copy m_dag to m_deviceDag
	 */
	void copyToGPU();

	/**
	 * Given the number of levels in the octree, this calculates the resolution.
	 */
	inline static size_t getResolution(size_t numLevels) {
		return 1 << (numLevels - 1);
	}

	/**
	 * Given a point in normalized device coordinates, this calculates 
	 * the corresponding discrete coordinates within a 3-dimensional grid.
	 */
	inline static ivec3 getPathFromNDC(vec3 ndc, size_t numLevels) {
		int resolution = getResolution(numLevels) - 1;
		ndc += vec3(1.0f, 1.0f, 1.0f);
		ndc *= 0.5f;
		return ivec3(ndc.x * resolution, ndc.y * resolution, ndc.z * resolution);
	}

	/**
	 * Constructs the sparse voxel octree in a 1-dimensional array.
	 * The resulting datastructure is not compressed, i.e. every node has 8 pointers even if they
	 * are 0.
	 * @see compress
	 * @see mergeSubtrees
	 * @return Returns offsets to all levels for further processing.
	 */
	vector<uint> constructSvo(const MinMaxHierarchy& minMax);

	/**
	 * Constructs the last 3 levels of the SVO using 64-bit leafmasks.
	 */
	void constructLastLevels(const MinMaxHierarchy& minMax, size_t levelOffset, size_t numNodes,
			const vector<ivec3>& childCoords);

	/**
	 * Merges common subtrees of an SVO to transform it into a directed acyclic graph (DAG).
	 * @note Assumes an uncompressed SVO exists.
	 */
	void mergeCommonSubtrees(const vector<uint>& levelOffsets);

	/**
	 * Helper function for merging common subtrees which updates the child pointers of the parent level
	 * according to a given mapping.
	 */
	void updateParentPointers(const vector<uint>& levelOffsets, unordered_map<uint, uint>& mapping, uint parentLevel);

	/**
	 * Helper function which removes unused, i.e. all zero nodes from the DAG.
	 */
	void removeUnusedNodes(const vector<uint>& levelOffsets, const vector<uint>& numNodesPerLevel);

	/**
	 * During the construction of the SVO/DAG each node will have 8 pointers to its children.
	 * This function will compress this structure by removing all unnecessary pointers.
	 */
	void compress();

private:
	/* Private class members */
	size_t m_numLevels;

	vector<uint> m_dag;

	unique_ptr<SSBO> m_deviceDag;

	ShaderProgram m_traverseCS;
};

#endif
