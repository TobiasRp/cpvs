#version 440

in vec4 vertex;
in vec2 texcoords;

out vec2 position;

void main(void)
{
    gl_Position = vertex;
    position = texcoords;
}
