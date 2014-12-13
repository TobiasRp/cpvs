#version 440 core

in vec2 position;
out vec4 fragColor;

layout(binding=0) uniform sampler2D positionBuffer;
layout(binding=1) uniform sampler2D normalBuffer;
layout(binding=2) uniform sampler2D diffuseBuffer;

struct Light {
	vec3 color;
	vec3 direction;
};

uniform Light light;

vec3 ambient_color = vec3(0.15, 0.15, 0.15);

void main(void) {
	vec4 positionTex = texture(positionBuffer, position);
	vec3 pos = positionTex.xyz;
	//float depth = positionTex.w;

	vec4 normalTex = texture(normalBuffer, position);
	vec3 N = normalTex.xyz;
	//float shininess = normalTex.w;

	vec3 L = - light.direction;
	vec4 diffuse_color = texture(diffuseBuffer, position);
	float diffuse = max(0.0, dot(N, L));

	vec3 scatteredLight = ambient_color + light.color * diffuse;

	fragColor = vec4(scatteredLight * diffuse_color.rgb, diffuse_color.a);
}
