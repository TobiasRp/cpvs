#version 440 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out VARYINGS {
	out vec3 position;
	out vec3 normal;
} vs_out;

uniform mat4 M;
uniform mat4 MVP;
uniform mat3 NormalMatrix;

void main(void)
{
	gl_Position = MVP * vec4(position, 1.0);
	vs_out.position = vec3(M * vec4(position, 1.0)).xyz;
	vs_out.normal = normalize(NormalMatrix * normal);
}
