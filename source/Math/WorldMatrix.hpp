#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

template<typename TRotation, typename TOffset>
struct TransformMatrix
{
protected:
	glm::vec<3, TRotation> angles = glm::vec<3, TRotation>(0.0);
	glm::vec<3, TOffset> offset = glm::vec<3, TOffset>(0.0);

public:
	template<typename Type = TOffset>
	glm::vec<3, Type> GetPosition() const
	{
		return glm::vec<3, Type>(offset);
	}

	template<typename Type = TRotation>
	glm::mat<3, 3, Type> GetOrientation() const
	{
		return glm::toMat3(glm::qua<TRotation>(angles));
	}

	template<typename Type = TRotation>
	glm::vec<3, Type> GetPitchYawRoll() const
	{
		return glm::vec<3, Type>(angles);
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

	template<typename Type>
	TransformMatrix& SetOffset(const glm::vec<3, Type>& V)
	{
		offset = glm::vec<3, TOffset>(V);
		return *this;
	}

	template<typename Type>
	TransformMatrix& SetOffset(Type x, Type y, Type z)
	{
		return SetOffset(glm::vec<3, TOffset>(x, y, z));
	}

	template<typename Type>
	TransformMatrix& SetRotation(const glm::mat<3, 3, Type>& M)
	{
		angles = glm::eulerAngles(glm::qua<TRotation>(M));
		return *this;
	}

	template<typename Type>
	TransformMatrix& SetRotation(Type pitch, Type yaw, Type roll)
	{
		angles = glm::vec<3, TRotation>(pitch, yaw, roll);
		return *this;
	}

	template<typename Type>
	TransformMatrix& SetRotation(const glm::vec<3, Type>& R, const glm::vec<3, Type>& U, const glm::vec<3, Type>& F)
	{
		glm::mat<3, 3, TRotation> m;
		m[0] = glm::vec<3, TRotation>(R);
		m[1] = glm::vec<3, TRotation>(U);
		m[2] = glm::vec<3, TRotation>(F);

		return SetRotation(m);
	}

	template<typename Type>
	TransformMatrix& SetRotation(const glm::vec<3, Type>& U, const glm::vec<3, Type>& F)
	{
		return SetRotation(glm::normalize(glm::cross(U, F)), U, F);
	}

	template<typename Type>
	TransformMatrix& Translate(const glm::vec<3, Type>& V)
	{
		return SetOffset(offset + glm::vec<3, TOffset>(GetOrientation() * V));
	}

	template<typename Type>
	TransformMatrix& Rotate(Type pitch, Type yaw, Type roll)
	{
		return SetRotation(angles.x + pitch, angles.y + yaw, angles.z + roll);
	}

	template<typename Type>
	TransformMatrix& SetFromMatrix(const glm::mat<4, 4, Type>& M)
	{
		SetRotation(glm::mat<3, 3, TRotation>(M));
		SetOffset(M[3][0], M[3][1], M[3][2]);
		return *this;
	}

	template<typename Type = TOffset>
	glm::mat<4, 4, Type> GetMatrix() const
	{
		glm::mat<4, 4, Type> m = glm::mat<4, 4, Type>(GetOrientation());
		return glm::translate(m, GetPosition());
	}
};

using WorldMatrix = TransformMatrix<float, double>; 
using LocalMatrix = TransformMatrix<float, double>;