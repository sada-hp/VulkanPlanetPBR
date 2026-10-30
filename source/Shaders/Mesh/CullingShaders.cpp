#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaders::CullCS)
(
	struct DrawCommand
	{
		uint indexCount;
		uint instanceCount;
		uint firstIndex;
		uint vertexOffset;
		uint firstInstance;
	};

	layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

	layout(set = 0, binding = 0) writeonly buffer indirectDraw 
	{
		DrawCommand at[];
	} commands;

	layout(set = 1, binding = 0) readonly buffer transform
	{
		mat4 at[];
	} matrices;

	layout(push_constant) uniform constants {
		// frustum
		vec4 planes[6];
		// count
		uint invocations;
	} data;

	uint frustumCull(in mat4 matrix)
	{
		vec4 point = matrix[3];

		for (uint i = 0; i < 6; i++)
		{
			if (dot(data.planes[i], point) < 0.0)
			{
				return 0;
			}
		}

		return 1;
	}

	void main()
	{
		if (gl_GlobalInvocationID.x < data.invocations)
		{
			mat4 tm = matrices.at[gl_GlobalInvocationID.x];
			// commands.at[gl_GlobalInvocationID.x].instanceCount = frustumCull(tm);
			commands.at[gl_GlobalInvocationID.x].instanceCount = 1;
		}
	}
);