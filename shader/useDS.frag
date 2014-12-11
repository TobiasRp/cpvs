#version 440

in vec2 position;
out vec4 fragColor;

layout(binding=0) uniform sampler2D positionBuffer;
layout(binding=1) uniform sampler2D normalBuffer;
layout(binding=2) uniform sampler2D diffuseBuffer;

struct Light {
	vec3 position;
};

uniform Light light;

uniform vec3 cameraPosition;

void main(void) {
	fragColor = vec4(texture2D(positionBuffer, position).xyz, 1.0);
}
