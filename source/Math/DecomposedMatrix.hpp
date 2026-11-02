#pragma once
#include "glm/glm.hpp"

struct DecomposedMatrix
{
	glm::mat3 rotation;
	glm::ivec3 offset_i;
	glm::vec3  offset_f;
};