layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

uniform mat4 LightShadowMatrices[3];

void main()
{
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = LightShadowMatrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}

	EndPrimitive();
}