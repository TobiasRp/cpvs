#ifndef COMPRESSED_SHADOW_H
#define COMPRESSED_SHADOW_H

#include "cpvs.h"

class MinMaxHierarchy;
class ShadowMap;

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
protected:
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
	/* Public member functions */

private:
	/* Private class members */
	unsigned m_width, m_height, m_depth;

};

#endif
