#pragma once
#include "EllipsoidMath.hpp"
#include "WorldMatrix.hpp"

struct EllipsoidMatrix : public GObject
{
	using TRotation = float;
	using TOffset = double;

private:
	const glm::vec<3, TOffset> position_base = glm::vec<3, TOffset>(0.0, GEllipsoid::Rg, 0.0);
	glm::mat<3, 3, TRotation> orientation_base = glm::mat<3, 3, TRotation>(1.0);
	glm::mat<4, 4, TOffset> matrix = glm::mat<4, 4, TOffset>(1.0);

protected:
	LocalMatrix local_offset;

private:
	void _update()
	{
		glm::vec<3, TOffset> p = position_base + local_offset.GetPosition();
		orientation_base = GEllipsoid::EulerFromCartesian(p);

		matrix = glm::mat<4, 4, TOffset>(orientation_base * local_offset.GetOrientation());
		matrix[3] = glm::vec<4, TOffset>(p, 1.0);

		invokeChange();
	}

public:
	EllipsoidMatrix()
	{
		local_offset.onChange(this, &EllipsoidMatrix::_update);
	}

	EllipsoidMatrix(const EllipsoidMatrix& other)
	{
		local_offset = other.local_offset;

		local_offset.onChange(this, &EllipsoidMatrix::_update);
		_update();
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
		return glm::vec<3, Type>(matrix[3]);
	}

	template<typename Type = TRotation>
	glm::mat<3, 3, Type> GetPlanetOrientation() const
	{
		return orientation_base;
	}

	template<typename Type = TRotation>
	glm::mat<3, 3, Type> GetOrientation() const
	{
		return glm::mat<3, 3, Type>(matrix);
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

	template<typename Type = TOffset>
	EllipsoidMatrix& SetPosition(const glm::vec<3, Type>& Off)
	{
		local_offset.SetOffset(Off - position_base);
		return *this;
	}

	template<typename Type = TRotation>
	EllipsoidMatrix& Rotate(Type pitch, Type yaw, Type roll)
	{
		local_offset.Rotate(pitch, yaw, roll);
		return *this;
	}

	const glm::mat<4, 4, TOffset>& GetMatrix() const
	{
		return matrix;
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