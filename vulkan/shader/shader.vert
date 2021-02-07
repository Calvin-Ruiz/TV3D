#version 440

layout (location=0) in vec4 position; // implicitly set z=0 and w=1
layout (location=1) in vec2 inTexCoords;

layout (location=0) out vec2 texCoord;

void main()
{
    gl_Position = position;
    texCoord = inTexCoords;
}
