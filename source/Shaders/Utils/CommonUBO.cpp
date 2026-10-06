#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderUtils::UBOCommon)
(
	layout(set = 0, binding = 0) uniform UnfiormBuffer
	{
		mat4 view_proj;
		mat4 view_proj_inv;

		vec4 sun_dir;
		vec4 eye_pos;
	} UBO;
);