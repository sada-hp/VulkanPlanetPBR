#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaders::MeshVS)
(
	layout(push_constant) uniform constants
	{
		layout(offset = 0) dmat4 Matrix;
	} Transform;

	layout(location = 0) in vec3 vertPosition;
	layout(location = 1) in vec3 vertNormal;
	layout(location = 2) in vec3 vertTangent;
	layout(location = 3) in vec2 vertUV;

	void main()
	{
		dvec4 WorldPositionFP64 = Transform.Matrix * dvec4(vertPosition, 1.0);
		gl_Position = vec4(UBO.view_proj * WorldPositionFP64);
	}
);

ShaderCodeDefinition(GShaders::MeshPS)
(
	layout(location = 0) out vec4 Color;

	void main()
	{
		Color = vec4(1.0, 0.0, 0.0, 1.0);
	}
);