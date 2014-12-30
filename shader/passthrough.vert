#version 440 core

in vec4 vertex;
in vec2 texcoords;

out vec2 texcoord;

void main(void)
{
    gl_Position = vertex;
    texcoord = texcoords;
}
