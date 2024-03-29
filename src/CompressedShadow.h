#ifndef COMPRESSED_SHADOW_H
#define COMPRESSED_SHADOW_H

#include "cpvs.h"

class MinMaxHierarchy;
class ShadowMap;

/**
 * This central datastructure of the CPVS represents the DAG of voxels
 * which is a compressed shadow of a light.
 *
 * The datastructure is built using a shadow map (actually it's min-max hierarchy).
 *
 * @see CompressedShadowContainer to evaluate one or more (tiled) precomputed shadows.
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
	CompressedShadow(uint numLevels);

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
	 * @param zTileIndex Index in [0, zTileNum) which specifies which z-tile to create.
	 * @param zTileNum Number of total z-tiles.
	 */
	static unique_ptr<CompressedShadow> create(const MinMaxHierarchy& minMax,
			uint zTileIndex = 0, uint zTileNum = 1);

	/*
	 * Creates a CompressedShadow from a shadow map.
	 * @note This will create a temporary min-max hierarchy.
	 */
	static unique_ptr<CompressedShadow> create(const ShadowMap* shadowMap,
			uint zTileIndex = 0, uint zTileNum = 1);

	/**
	 * Traverses the sparse voxel DAG (on the CPU) for the given position
	 * in normal device coordinates, i.e. in [-1, 1]^3.
	 *
	 * @note Useful for testing purposes!
	 */
	NodeVisibility traverse(const vec3 position, bool tryLeafmasks = true);

	/**
	 * Returns the visibility of the whole shadow.
	 */
	NodeVisibility getTotalVisibility() const;

	inline uint getNumLevels() const {
		return m_numLevels;
	}

	inline const vector<uint>& getDAG() const {
		return m_dag;
	}
	
private:
	/* Private member and helper functions */

	/**
	 * Constructs the sparse voxel octree in a 1-dimensional array.
	 * The resulting datastructure is not compressed, i.e. every node has 8 pointers even if they
	 * are 0.
	 * @see compress
	 * @see mergeCommonSubtrees
	 * @return Returns offsets to all levels for further processing.
	 */
	vector<uint> constructSvo(const MinMaxHierarchy& minMax, const ivec3 rootOffset);

	/**
	 * Constructs the last 3 levels of the SVO using 64-bit leafmasks.
	 */
	void constructLastLevels(const MinMaxHierarchy& minMax, size_t levelOffset, size_t numNodes,
			const vector<ivec3>& childCoords);

	/**
	 * Merges common subtrees of an SVO to transform it into a directed acyclic graph (DAG).
	 * @note Assumes an uncompressed SVO.
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
	uint m_numLevels;

	vector<uint> m_dag;
};

#endif
