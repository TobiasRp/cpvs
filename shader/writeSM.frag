#version 440 core

layout (location = 0) out vec4 fragColor;

in vec2 position;

layout (binding  = 0) uniform sampler2D imageTex;

void main() {
	//fragColor = texture(imageTex, position);

	vec4 t = texture(imageTex, position);
	if (t.x == 1.0)
		fragColor = vec4(1.0);
	else {
		fragColor = vec4(t.x * 0.33 - 0.8);
	}
}
