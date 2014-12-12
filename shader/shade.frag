#version 440 core

in vec2 position;
out vec4 fragColor;

layout(binding=0) uniform sampler2D positionBuffer;
layout(binding=1) uniform sampler2D normalBuffer;
layout(binding=2) uniform sampler2D diffuseBuffer;

layout(binding=3) uniform sampler2D shadowMap;

/* Determines if and how shadow is rendered.
 * 0 - no shadow
 * 1 - shadow mapping
 */
uniform int shadowMode;
float simpleBias = 2.0;
uniform mat4 lightVP;

struct Light {
	vec3 color;
	vec3 direction;
};

uniform Light light;

vec3 ambient_color = vec3(0.15, 0.15, 0.15);

void main(void) {
	vec4 positionTex = texture(positionBuffer, position);
	vec3 pos = positionTex.xyz;
	float depth = positionTex.w;

	float lit = 1.0;
	if (shadowMode == 1) {
		vec4 shadowCoord = lightVP * vec4(pos, 1.0);
		shadowCoord = shadowCoord / shadowCoord.w;
		float shadow = texture(shadowMap, shadowCoord.xy).x;

		if (depth > shadow + simpleBias)
			lit = 0.0;
	}

	vec4 normalTex = texture(normalBuffer, position);
	vec3 N = normalTex.xyz;
	//float shininess = normalTex.w;

	vec4 diffuse_color = texture(diffuseBuffer, position);
	float diffuse = max(0.0, dot(N, light.direction));

	vec3 scatteredLight = ambient_color + lit * light.color * diffuse;

	fragColor = vec4(scatteredLight * diffuse_color.rgb, diffuse_color.a);
}
