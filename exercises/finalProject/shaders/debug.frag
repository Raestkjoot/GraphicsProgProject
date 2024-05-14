#version 330 core

in vec4 Color;

out vec4 FragColor;

void main()
{
	FragColor = vec4(Color.rgb, 1.0f);
}
