#version 440 core

layout (location = 0) out float frag;

void main(void) {
	frag = gl_FragCoord.z;
}
