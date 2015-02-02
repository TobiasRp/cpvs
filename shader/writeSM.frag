#version 440 core

layout (location = 0) out vec4 fragColor;

in vec2 texcoord;

layout (binding  = 0) uniform sampler2D imageTex;

uniform float znear;
uniform float zfar;

float linearizeDepth(in vec2 uv) {
	float depth = texture(imageTex, uv).r;
	return (1.0 * znear) / (zfar + znear - depth * (zfar - znear));
}

void main() {
	float d = linearizeDepth(texcoord) - 0.4; // make it darker for better visualization
	fragColor = vec4(d, d, d, 1.0);
}
