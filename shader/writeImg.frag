#version 440 core

layout (location = 0) out vec4 fragColor;

in vec2 position;

layout (binding  = 0) uniform sampler2D imageTex;

void main() {
	fragColor = texture(imageTex, position);
}
