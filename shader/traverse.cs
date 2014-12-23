#version 440 core

layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba32f, binding = 0) uniform image2D positionsWS;
layout (rg8, binding = 1) uniform image2D visibilities;

uniform mat4 shadowProj;

layout (std430, binding = 2) buffer shadowDag {
	uint dag[];
};

void main() {
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	imageStore(visibilities, pos, vec4(0.5));
}
