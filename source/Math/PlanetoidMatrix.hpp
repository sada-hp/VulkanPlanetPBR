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
	glm::mat<3, 3, Type> GetOrientation() const
	{
		glm::mat<3, 3, TRotation> planet_orientation = GEllipsoid::EulerFromCartesian(GetPosition());
		return planet_orientation * local_offset.GetOrientation();
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
	EllipsoidMatrix& Translate(const glm::vec<3, Type>& V)
	{
		local_offset.SetOffset(local_offset.GetPosition<Type>() + GetOrientation<Type>() * V);
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
		glm::mat<4, 4, Type> m = glm::mat<4, 4, Type>(GetOrientation());
		return glm::translate(m, GetPosition());
	}
};