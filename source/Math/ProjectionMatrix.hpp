#pragma once
#include "glm/glm.hpp"

struct ProjectionMatrix
{
protected:
	glm::mat4 matrix = glm::mat4(glm::vec4(0.f), glm::vec4(0.f), glm::vec4(0.f, 0.f, 0.f, -1.f), glm::vec4(0.f));

public:
	ProjectionMatrix& SetDepthRange(float Near, float Far)
	{
#if 0
		// From 0 to 1
		matrix[2][2] = -Far / (Far - Near);
		matrix[3][2] = -(Far * Near) / (Far - Near);
#else
		// Reversed depth
		matrix[2][2] = Near / (Far - Near);
		matrix[3][2] = (Far * Near) / (Far - Near);
#endif
		return *this;
	}

	ProjectionMatrix& SetFOV(float Fov)
	{
		const float tanHalfFovy = glm::tan(glm::radians(Fov) / 2.f);
		const float aspect = matrix[0][0] / (matrix[1][1] == 0.0 ? 1.0 : -matrix[1][1]);

		matrix[0][0] = aspect / tanHalfFovy;
		matrix[1][1] = -1.f / tanHalfFovy;

		return *this;
	}

	ProjectionMatrix& SetAspect(float Aspect)
	{
		matrix[0][0] = (matrix[1][1] == 0.0 ? 1.0 : -matrix[1][1]) / Aspect;

		return *this;
	}

	glm::mat4 GetMatrix() const
	{
		glm::mat4 m = matrix;
#if 0
		m[2][2] = 0.0;
		m[3][2] = -1.0;
#endif
		return m;
	}
};