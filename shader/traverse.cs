#version 440 core

#define LOCAL_SIZE 32

layout (local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE) in;

uniform uint width;
uniform uint height;

uniform int num_levels;

uniform mat4 lightViewProj;

layout (rgba32f, binding = 0) uniform image2D positionsWS;
layout (r8, binding = 1)      uniform image2D visibilities;

layout (std430, binding = 2) buffer shadowDag {
	uint dag[];
};

const int RESOLUTION = 1 << (num_levels - 1);

ivec3 getPathFromNDC(vec3 ndc) {
	uint max = RESOLUTION - 1;
	ndc += vec3(1.0, 1.0, 1.0);
	ndc *= 0.5f;
	return ivec3(ndc.x * max, ndc.y * max, ndc.z * max);
}

float testLeafmask(ivec3 path, uint lowerHalf, uint upperHalf) {
	int index = (path.x & 0x3) + 4 * (path.y & 0x3) + 16 * (path.z & 0x3);

	if (index < 31) {
		return float(lowerHalf & (1 << index));
	} else {
		return float(upperHalf & (1 << index));
	}
}

float traverse(const vec3 projPos) {
	ivec3 path = getPathFromNDC(projPos);
	uint offset = 0;
	int level = num_levels - 2;

	while(level >= 0) {
		uint lvlBit = 1 << (level);
		uint childIndex = (bool(path.x & lvlBit) ? 2 : 0) +
						  (bool(path.y & lvlBit) ? 4 : 0) +
						  (bool(path.z & lvlBit) ? 8 : 0);

		uint childmask = dag[offset];

		uint visibility = 0x3 & (childmask >> childIndex);
		if (visibility == 0)
			return 0.0;
		else if (visibility == 1)
			return 1.0;

		uint maskedChildmask = childmask & (0xAAAA >> (16 - childIndex));
		uint childOffset = bitCount(maskedChildmask);

//		if (level == 2) {
//			// Test visibility using the 64-bit leafmask, encoded as two 32-bit values
//			uint index = offset + childOffset * 2 + 1;
//			return testLeafmask(path, dag[index], dag[index + 1]);
//		}

		offset = dag[offset + 1 + childOffset];

		level -= 1;
	}
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
