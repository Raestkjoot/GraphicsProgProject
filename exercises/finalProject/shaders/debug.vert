#version 330 core

//Inputs
layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec4 VertexColor;

out vec4 Color;

//Uniforms
uniform mat4 WorldViewProjMatrix;

vec4 GetVertexColor(uint inColor)
{
	vec4 color;

	color.r = ((inColor & 0xff000000u) >> 24) / 255.0f;
	color.g = ((inColor & 0x00ff0000u) >> 16) / 255.0f;
	color.b = ((inColor & 0x0000ff00u) >> 8) / 255.0f;
	color.a = ((inColor & 0x000000ffu)) / 255.0f;

	return color;
}

void main()
{
	gl_Position = WorldViewProjMatrix * vec4(VertexPosition, 1.0);
	Color = VertexColor;
}