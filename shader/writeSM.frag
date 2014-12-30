#version 440 core

layout (location = 0) out vec4 fragColor;

in vec2 texcoord;

layout (binding  = 0) uniform sampler2D imageTex;

float linearizeDepth(in vec2 uv) {
	const float znear = 1.0;
	const float zfar = 1000;
	float depth = texture(imageTex, uv).r;
	return (1.0 * znear) / (zfar + znear - depth * (zfar - znear));
}

void main() {
	float d = linearizeDepth(texcoord);
	fragColor = vec4(d, d, d, 1.0);
}
