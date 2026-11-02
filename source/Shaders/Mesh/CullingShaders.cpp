#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaders::CullCS)
(
\n #extension GL_EXT_scalar_block_layout : enable \n

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
		mat3 rotation;
		
		ivec3 positioni;
		vec3  positionf;
		
		vec3 min;
		vec3 max;
	};

	layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

	layout(set = 0, binding = 0) writeonly buffer indirectDraw 
	{
		DrawCommand at[];
	} commands;

	layout(scalar, set = 1, binding = 0) readonly buffer mesh_data
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
		MeshData Mesh = descriptors.at[gl_GlobalInvocationID.x];

		mat4 matrix = mat4(Mesh.rotation);
		matrix[3].xyz = vec3(Mesh.positioni.xyz) + Mesh.positionf.xyz;

		vec3 min = Mesh.min.xyz;
		vec3 max = Mesh.max.xyz;

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