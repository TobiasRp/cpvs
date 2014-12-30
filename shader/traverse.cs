#version 440 core

#define LOCAL_SIZE 32

layout (local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE) in;

uniform uint width;
uniform uint height;

uniform mat4 lightViewProj;

layout (rgba32f, binding = 0) uniform image2D positionsWS;
layout (r8, binding = 1)      uniform image2D visibilities;

layout (std430, binding = 2) buffer shadowDag {
	uint dag[];
};

/* Fix size of shadow here! Be sure to update if the size changes */
const int NUM_LEVELS = 14;

const int RESOLUTION = 1 << (NUM_LEVELS - 1);

ivec3 getPathFromNDC(vec3 ndc) {
	uint max = RESOLUTION - 1;
	ndc += vec3(1.0, 1.0, 1.0);
	ndc *= 0.5f;
	return ivec3(ndc.x * max, ndc.y * max, ndc.z * max);
}

/* GLSL doesn't seem to support the popcount instruction, but the hardware probably does...
 * Emulate it for the time being.
 */
uint popcount(uint x) {
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0F0F0F0F;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return x & 0x0000003F;
}

float traverse(const vec3 projPos) {
	ivec3 path = getPathFromNDC(projPos);
	uint offset = 0;
	int level = NUM_LEVELS;

	while(level > 1) {
		uint lvlBit = 1 << (level - 2);
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
		uint childOffset = popcount(maskedChildmask);
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
