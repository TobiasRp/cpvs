#version 440 core

in VARYINGS {
	vec3 position;
	vec3 normal;
} fs_in;


struct MaterialParameter {
	vec3 diffuse_color;
	int shininess;
};

uniform MaterialParameter material;

out vec4 fs_output;

void main() {
	fs_output = vec4(material.diffuse_color, 1.0);
}
