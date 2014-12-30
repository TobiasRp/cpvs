#version 440 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 MVP;

// UNUSED
out VARYINGS {
	out vec4 position;
	out vec3 normal;
} vs_out;

void main()
{
	gl_Position = MVP * vec4(position, 1.0);
}
