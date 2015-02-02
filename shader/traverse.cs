#version 440 core

#define LOCAL_SIZE 32

// If defined leafmasks are used. Also has to be modified in CompressedShadow.cpp
#define LEAFMASKS

layout (local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE) in;

uniform uint width;
uniform uint height;

uniform int dag_levels;
uniform int grid_levels;

uniform mat4 lightViewProj;

layout (rgba32f, binding = 0) uniform image2D positionsWS;
layout (r8, binding = 1)      uniform image2D visibilities;

layout (std430, binding = 2) buffer shadowDAG {
	uint dag[];
};

layout (std430, binding = 3) buffer topLevelGrid {
	uint grid[];
};

#ifdef LEAFMASKS
const int MIN_LEVEL = 3;
#else
const int MIN_LEVEL = 0;
#endif

const int RESOLUTION = 1 << (dag_levels + grid_levels - 1);

ivec3 getPathFromNDC(vec3 ndc) {
	uint max = RESOLUTION - 1;
	ndc += vec3(1.0, 1.0, 1.0);
	ndc *= 0.5f;
	return ivec3(ndc.x * max, ndc.y * max, ndc.z * max);
}

/* Returns the visibility from the given Leafmask */
float testLeafmask(ivec3 path, uint lowerHalf, uint upperHalf) {
	uint index = (path.x & 0x7) + 8 * (path.y & 0x7);

	uint vis;
	if (index < 32) {
		vis = lowerHalf & (1 << index);
	} else {
		vis = upperHalf & (1 << (index - 32));
	}
	return float(vis);
}

uint getChildOffset(uint childmask, uint childIndex) {
	uint maskedChildmask = childmask & (0xAAAA >> (16 - childIndex));
	return bitCount(maskedChildmask);
}

/* Traverses the precomputed shadow for the given vector in NDC, i.e. in the range [-1,1]^3 */
float traverse(const vec3 projPos) {
	ivec3 path = getPathFromNDC(projPos);
	uint offset = 0;

	uint grid_res = 1 << grid_levels;
	ivec3 gridCoords = path >> (dag_levels - 1);
	uint dagOffset = grid[gridCoords.z * grid_res * grid_res + gridCoords.y * grid_res + gridCoords.x];
	offset = dagOffset;

	// The grid stores two special values which indicate if the entire grid cell is
	// visible or in shadow.
	if (offset == 0xFFFFFFFF)
		return 0.0; // shadow
	if (offset == 0xFFFFFFFE)
		return 1.0; // visible

	int level = dag_levels - 2;
	while(level >= MIN_LEVEL) {
		uint lvlBit = 1 << level;
		uint childIndex = (bool(path.x & lvlBit) ? 2 : 0) +
						  (bool(path.y & lvlBit) ? 4 : 0) +
						  (bool(path.z & lvlBit) ? 8 : 0);

		uint childmask = dag[offset];

		uint visibility = 0x3 & (childmask >> childIndex);
		if (visibility == 0)
			return 0.0;
		else if (visibility == 1)
			return 1.0;

		uint childOffset = getChildOffset(childmask, childIndex);

		offset = dagOffset + dag[offset + 1 + childOffset];

		level -= 1;
	}

#ifdef LEAFMASKS
	{
		// level == 2, so evaluate 1x1x8 and if necessary the 8x8x1 64-bit leafmask
		uint childIndex = (path.z & 0x7) * 2;
		uint childmask  = dag[offset];

		uint visibility = 0x3 & (childmask >> childIndex);
		if (visibility == 0)
			return 0.0;
		else if (visibility == 1)
			return 1.0;

		uint childOffset = getChildOffset(childmask, childIndex);

		// Test visibility using the 64-bit leafmask, encoded as two 32-bit values
		uint index = offset + childOffset * 2 + 1;
		return testLeafmask(path, dag[index], dag[index + 1]);
	}
#endif

	return 1.0;
}

void main() {
	ivec2 index = ivec2(gl_GlobalInvocationID.xy);

	if (index.x >= width || index.y >= height)
		return;

	vec3 posWS = imageLoad(positionsWS, index).xyz;

	vec4 projPos = lightViewProj * vec4(posWS, 1.0);
	projPos = projPos / projPos.w;

	float vis = traverse(projPos.xyz);
	imageStore(visibilities, index, vec4(vis));
}
