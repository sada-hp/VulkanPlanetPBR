#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaders::FullscreenVS)
(
	layout(location = 0) out vec2 UV;

	void main()
	{
		UV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
		gl_Position = vec4(UV * 2.0f - 1.0f, 0.0f, 1.0f);
	}
);

ShaderCodeDefinition(GShaders::FullscreenLayeredVS)
(
	layout(location = 0) out vec3 UVW;

	void main()
	{
		UVW.xy = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
		UVW.z = gl_InstanceIndex;

		gl_Position = vec4(UVW.xy * 2.0f - 1.0f, 0.0f, 1.0f);
	}
);