#version 440 core

layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba32f, binding = 0) uniform image2D positionsWS;
layout (rg8, binding = 1) uniform image2D visibilities;

uniform mat4 shadowProj;

layout (std430, binding = 2) buffer shadowDag {
	uint dag[];
};

const int NUM_LEVELS = 12;
const int RESOLUTION = 1 << (NUM_LEVELS - 1);

ivec3 getPathFromNDC(vec3 ndc) {
	ndc += vec3(1.0, 1.0, 1.0);
	ndc *= 0.5f;
	return ivec3(ndc.x * RESOLUTION, ndc.y * RESOLUTION, ndc.z * RESOLUTION);
}

/* GLSL doesn't seem to support the popcount instruction, but the hardware probably does...
 * Emulate it for the time being.
 */
uint popcount(uint x) {
    x = x - ((x >> 1) & 0x5555);
    x = (x & 0x3333) + ((x >> 2) & 0x3333);
    x = (x + (x >> 4)) & 0x0F0F;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return x & 0x003F;
}

bool traverse(const vec3 projPos) {
	ivec3 path = getPathFromNDC(projPos);
	uint offset = 0;
	int level = NUM_LEVELS;

	while(level > 1) {

		int lvlBit = 1 << (level - 2);
		int childIndex = (bool(path.x & lvlBit) ? 2 : 0) +
						 (bool(path.y & lvlBit) ? 4 : 0) +
						 (bool(path.z & lvlBit) ? 8 : 0);

		uint childmask = dag[offset];

		uint visibility = 0x3 & (childmask >> childIndex);
		if (visibility == 0)
			return false;
		else if (visibility == 1)
			return true;

		uint maskedChildmask = childmask & (0xAAAA >> (16 - childIndex));
		uint childOffset = popcount(maskedChildmask);
		offset = dag[offset + 1 + childOffset];

		level -= 1;
	}
}

void main() {
	ivec2 index = ivec2(gl_GlobalInvocationID.xy);

	vec4 posWS = imageLoad(positionsWS, index);

	vec4 projPos = shadowProj * posWS;
	projPos.xyz = projPos.xyz / projPos.z;

	float vis = traverse(projPos.xyz) ? 1.0 : 0.0;
	imageStore(visibilities, index, vec4(vis));
}
