#pragma once
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/glm.hpp"

namespace GEllipsoid
{
	constexpr double Rg = 6360 * 1e3;
	constexpr double Rt = 6420 * 1e3;

	template<typename T>
	glm::vec<3, T> CartesianFromGeo(T longitude, T latitude, T height)
	{
		T lonRad = glm::radians(longitude);
		T latRad = glm::radians(latitude);

		T CosLon = glm::cos(lonRad);
		T SinLon = glm::sin(lonRad);
		T CosLat = glm::cos(latRad);
		T SinLat = glm::sin(latRad);

		glm::vec<3, T> n = glm::vec<3, T>(CosLat * CosLon, SinLat, CosLat * SinLon);
		glm::vec<3, T> k = n * static_cast<T>(Rg) * static_cast<T>(Rg);
		T gamma = glm::sqrt(k.x * n.x + k.y * n.y + k.z * n.z);

		return k / gamma + (n * height);
	}

	template<typename T>
	glm::mat<3, 3, T> EulerFromCartesian(glm::vec<3, T> offset)
	{
		glm::vec<3, T> up = glm::normalize(offset);
		glm::qua<T> orientation = glm::rotation(glm::vec<3, T>(0, 1, 0), up);
		return glm::mat3_cast(orientation);
	}

	template<typename T>
	glm::mat<4, 4, T> TransformFromCartesian(glm::vec<3, T> offset)
	{
		glm::mat<4, 4, T> transform = EulerFromCartesian(offset);
		transform = glm::translate(transform, offset);
		return transform;
	}

	template<typename T>
	glm::mat<4, 4, T> TransformFromGeo(T longitude, T latitude, T height)
	{
		glm::vec<3, T> offset = CartesianFromGeo(longitude, latitude, height, Rg);
		return TransformFromCartesian(offset);
	}

	template<typename T>
	glm::vec<4, T> CartesianToGeo(glm::vec<3, T> offset)
	{
		glm::vec<4, T> Out = glm::vec<4, T>(0, 0, 0, Rg);

		Out.x = glm::degrees(glm::atan(offset.z, offset.x));
		Out.y = glm::degrees(glm::asin(glm::clamp(0.0, 1.0, offset.y / Rg)));
		Out.z = glm::length(offset) - Rg;

		return Out;
	}
};