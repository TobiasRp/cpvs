#version 440 core

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

uniform float znear, zfar;

layout (location = 0) in vec4 position;

void main() {
	vec4 posLV = V * M * position;
	gl_Position = P * posLV;

	// Linearizes z-buffer (see Real-Time Shadows, p.37)
	// ... but is not working correctly?
	//gl_Position.z = - ((posLV.z + znear) / (zfar - znear)) * gl_Position.w;
}
