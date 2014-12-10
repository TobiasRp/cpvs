#version 440 core

in VARYINGS {
	vec3 position;
	vec3 normal;
} fs_in;

out vec4 fs_output;

void main() {
	fs_output = vec4(0.4, 0.4, 0.4, 1.0);
}
