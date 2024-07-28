layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

//layout (std140) uniform LightSpaceMatrices
//{
//	mat4 lightSpaceMatrices[3];
//}

void main()
{
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = gl_in[i].gl_Position;
		EmitVertex();
	}

	EndPrimitive();
}