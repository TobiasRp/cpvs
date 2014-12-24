#version 440 core

in vec2 position;
out vec4 fragColor;

layout(binding=0) uniform sampler2D positionBuffer;
layout(binding=1) uniform sampler2D normalBuffer;
layout(binding=2) uniform sampler2D diffuseBuffer;
layout(binding=3) uniform sampler2D visibilities;
uniform int renderShadow;

struct Light {
	vec3 color;
	vec3 direction;
};

uniform Light light;

vec3 ambient_color = vec3(0.15, 0.15, 0.15);
//vec3 ambient_color = vec3(0.0, 0.0, 0.0);

void main(void) {
	float vis = 1.0f;
	if (renderShadow != 0) {
		vis = texture(visibilities, position).r;
	}

	vec4 positionTex = texture(positionBuffer, position);
	vec3 pos = positionTex.xyz;
	//float depth = positionTex.w;

	vec4 normalTex = texture(normalBuffer, position);
	vec3 N = normalize(normalTex.xyz);
	//float shininess = normalTex.w;

	vec3 L = normalize( - light.direction);
	vec4 diffuse_color = texture(diffuseBuffer, position);
	float diffuse = max(0.0, dot(N, L));

	vec3 scatteredLight = ambient_color + light.color * diffuse;

	fragColor = vec4(vis * scatteredLight * diffuse_color.rgb, diffuse_color.a);
}
