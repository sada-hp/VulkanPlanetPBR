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

	struct MeshData
	{
		mat4 transform;
		vec4 min;
		vec4 max;
	};

	layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

	layout(set = 0, binding = 0) writeonly buffer indirectDraw 
	{
		DrawCommand at[];
	} commands;

	layout(set = 1, binding = 0) readonly buffer mesh_data
	{
		MeshData at[];
	} descriptors;

	layout(push_constant) uniform constants {
		// frustum
		vec4 planes[6];
		// count
		uint invocations;
	} data;

	uint frustumCull()
	{
		mat4 matrix = descriptors.at[gl_GlobalInvocationID.x].transform;
		vec3 min = descriptors.at[gl_GlobalInvocationID.x].min.xyz;
		vec3 max = descriptors.at[gl_GlobalInvocationID.x].max.xyz;

		vec4 Box[8] = {
			matrix * vec4(min.x, min.y, min.z, 1.0),
			matrix * vec4(max.x, min.y, min.z, 1.0),
			matrix * vec4(min.x, max.y, min.z, 1.0),
			matrix * vec4(min.x, min.y, max.z, 1.0),
			matrix * vec4(max.x, max.y, min.z, 1.0),
			matrix * vec4(min.x, max.y, max.z, 1.0),
			matrix * vec4(max.x, min.y, max.z, 1.0),
			matrix * vec4(max.x, max.y, max.z, 1.0),
		};

		for (uint i = 0; i < 6; i++)
		{
			if (dot(data.planes[i], matrix[3]) < 0.0 &&
				dot(data.planes[i], Box[1]) < 0.0 &&
				dot(data.planes[i], Box[2]) < 0.0 &&
				dot(data.planes[i], Box[3]) < 0.0 &&
				dot(data.planes[i], Box[4]) < 0.0 &&
				dot(data.planes[i], Box[5]) < 0.0 &&
				dot(data.planes[i], Box[6]) < 0.0 &&
				dot(data.planes[i], Box[7]) < 0.0)
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
			commands.at[gl_GlobalInvocationID.x].instanceCount = frustumCull();
		}
	}
);