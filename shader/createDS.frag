#version 440 core

in VARYINGS {
	in vec3 position;
	in vec3 normal;
} fs_in;

struct MaterialParameter {
	vec3 diffuse_color;
	float shininess;
};

uniform MaterialParameter material;

layout (location = 0) out vec4 position;
layout (location = 1) out vec4 normal;
layout (location = 2) out vec4 diffuse;

void main(void)
{
    position = vec4(fs_in.position, gl_FragCoord.z);
    normal = vec4(normalize(fs_in.normal), material.shininess);
    diffuse = vec4(material.diffuse_color, 1.0);
}
