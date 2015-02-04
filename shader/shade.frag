#version 440 core

in vec2 texcoord;
out vec4 fragColor;

layout(binding=0) uniform sampler2D positionBuffer;
layout(binding=1) uniform sampler2D normalBuffer;
layout(binding=2) uniform sampler2D diffuseBuffer;
layout(binding=3) uniform sampler2D visibilities;
uniform int renderShadow;

layout(binding=4) uniform sampler2D shadowMap;
uniform mat4 lightViewProj;

struct Light {
	vec3 direction;
};

uniform Light light;

vec3 ambient_color = vec3(0.15, 0.15, 0.15);

void main(void) {
	vec4 positionTex = texture(positionBuffer, texcoord);
	vec3 pos = positionTex.xyz;

	float vis = 1.0f;
	if (renderShadow != 0) {
		vis = texture(visibilities, texcoord).r;
		vis = max(vis, 0.0);
	} else {
		vec4 shadowCoord = lightViewProj * vec4(pos, 1.0);
		shadowCoord /= shadowCoord.w;
		shadowCoord.xyz = shadowCoord.xyz * vec3(0.5, 0.5, 0.5) + vec3(0.5, 0.5, 0.5);
		float d = texture(shadowMap, shadowCoord.xy).r;

		if (d < shadowCoord.z)
			vis = 0.0;
	}

	vec4 normalTex = texture(normalBuffer, texcoord);
	vec3 N = normalTex.xyz;

	vec3 L = normalize(light.direction);
	vec4 diffuse_color = texture(diffuseBuffer, texcoord);
	float diffuse = max(0.0, dot(N, L));

	vec3 scatteredLight = ambient_color + diffuse * vis;

	fragColor = vec4(scatteredLight * diffuse_color.rgb, diffuse_color.a);
}
