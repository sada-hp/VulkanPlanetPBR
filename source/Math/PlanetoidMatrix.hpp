#pragma once
#include "EllipsoidMath.hpp"
#include "WorldMatrix.hpp"

struct EllipsoidMatrix
{
	using TRotation = float;
	using TOffset = double;

protected:
	const glm::vec<3, TOffset> planet_base = glm::vec<3, TOffset>(0.0, GEllipsoid::Rg, 0.0);
	LocalMatrix local_offset;

public:
	EllipsoidMatrix() = default;

	EllipsoidMatrix(const EllipsoidMatrix& other)
	{
		local_offset = other.local_offset;
	}

	LocalMatrix& GetLocalMatrix()
	{
		return local_offset;
	}

	const LocalMatrix& GetLocalMatrix() const
	{
		return local_offset;
	}

	template<typename Type = TOffset>
	glm::vec<3, Type> GetPosition() const
	{
		return planet_base + local_offset.GetPosition();
	}

	template<typename Type = TRotation>
	glm::mat<3, 3, Type> GetPlanetOrientation() const
	{
		return GEllipsoid::EulerFromCartesian(GetPosition());
	}

	template<typename Type = TRotation>
	glm::mat<3, 3, Type> GetOrientation() const
	{
		return GetPlanetOrientation() * local_offset.GetOrientation();
	}

	template<typename Type = TOffset>
	glm::vec<3, Type> GetForward() const
	{
		return glm::vec<3, Type>(GetOrientation()[2]);
	}

	template<typename Type = TOffset>
	glm::vec<3, Type> GetRight() const
	{
		return glm::vec<3, Type>(GetOrientation()[0]);
	}

	template<typename Type = TOffset>
	glm::vec<3, Type> GetUp() const
	{
		return glm::vec<3, Type>(GetOrientation()[1]);
	}

	template<typename Type = TOffset>
	EllipsoidMatrix& Move(const glm::vec<3, Type>& V)
	{
		local_offset.SetOffset(local_offset.GetPosition<Type>() + GetOrientation<Type>() * V);
		return *this;
	}

	template<typename Type = TOffset>
	EllipsoidMatrix& Translate(const glm::vec<3, Type>& V)
	{
		local_offset.SetOffset(local_offset.GetPosition<Type>() + GetPlanetOrientation<Type>() * V);
		return *this;
	}

	template<typename Type = TRotation>
	EllipsoidMatrix& Rotate(Type pitch, Type yaw, Type roll)
	{
		local_offset.Rotate(pitch, yaw, roll);
		return *this;
	}

	template<typename Type = TOffset>
	glm::mat<4, 4, Type> GetMatrix() const
	{
		glm::vec<3, Type> p = planet_base + local_offset.GetPosition();
		glm::mat<3, 3, Type> m3 = GEllipsoid::EulerFromCartesian(p);
		m3 = m3 * local_offset.GetOrientation<Type>();

		glm::mat<4, 4, Type> m4 = glm::mat<4, 4, Type>(m3);
		return glm::translate(m4, p);
	}

	template<typename Type = TOffset>
	EllipsoidMatrix& LookAt(const glm::vec<3, Type>& target)
	{
		glm::mat<3, 3, Type> planet_orientation = GetPlanetOrientation();
		glm::vec<3, Type> eye = GetPosition();

		glm::vec<3, Type> fwd = glm::normalize(target - eye);
		glm::vec<3, Type> up = planet_orientation[1];

		glm::qua<Type> q = glm::quatLookAt(-fwd, up);
		local_offset.SetRotation(glm::transpose(planet_orientation) * glm::mat3_cast(q));
		return *this;
	}
};