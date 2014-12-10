#version 440 core

in VARYINGS {
	vec3 position;
	vec3 normal;
} fs_in;

uniform vec3 diffuse_color;

out vec4 fs_output;

void main() {
	fs_output = vec4(diffuse_color, 1.0);
}
