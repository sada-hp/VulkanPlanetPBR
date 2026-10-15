#pragma once
#include "glm/glm.hpp"

using namespace glm;

#define UBO_contents \
mat4 view_proj; \
mat4 view_proj_no_offset; \
mat4 view_proj_inv; \
vec4 sun_dir; \
vec4 eye_pos; \
float time; \

struct _UniformBuffer
{
	UBO_contents
};