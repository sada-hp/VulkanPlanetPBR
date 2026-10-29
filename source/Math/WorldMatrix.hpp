#pragma once
#include "Engine/object.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

template<typename TRotation, typename TOffset>
struct TransformMatrix : public GObject
{
	friend class EllipsoidMatrix;

protected:
	glm::mat<4, 4, TOffset> matrix = glm::mat<4, 4, TOffset>(1.0);

protected:
	glm::vec<3, TRotation> angles = glm::vec<3, TRotation>(0.0);
	glm::vec<3, TOffset> offset = glm::vec<3, TOffset>(0.0);

protected:
	void _update()
	{
		matrix = glm::toMat4(glm::qua<TOffset>(glm::radians(angles)));
		matrix[3] = glm::vec<4, TOffset>(offset, 1.0);

		invokeChange();
	}

public:
	TransformMatrix() = default;
	TransformMatrix(const TransformMatrix& other)
		: GObject(other)
	{
		matrix = other.matrix;
		angles = other.angles;
		offset = other.offset;
	}

	template<typename Type = TOffset>
	glm::vec<3, Type> GetPosition() const
	{
		return glm::vec<3, Type>(offset);
	}

	template<typename Type = TRotation>
	glm::mat<3, 3, Type> GetOrientation() const
	{
		return glm::mat<3, 3, Type>(matrix);
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
		_update();

		return *this;
	}

	template<typename Type>
	TransformMatrix& SetOffset(Type x, Type y, Type z)
	{
		return SetOffset(glm::vec<3, TOffset>(x, y, z));
	}

	template<typename Type>
	TransformMatrix& SetRotation(const glm::qua<Type>& Q)
	{
		angles = glm::degrees(glm::eulerAngles(Q));
		_update();

		return *this;
	}

	template<typename Type>
	TransformMatrix& SetRotation(const glm::mat<3, 3, Type>& M)
	{
		return SetRotation(glm::qua<TRotation>(M));
	}

	template<typename Type>
	TransformMatrix& SetRotation(Type pitch, Type yaw, Type roll)
	{
		angles = glm::vec<3, TRotation>(pitch, yaw, roll);
		_update();

		return *this;
	}

	template<typename Type>
	TransformMatrix& SetRotation(const glm::vec<3, Type>& F, const glm::vec<3, Type>& U)
	{
		return SetRotation(glm::quatLookAt(-F, U));
	}

	template<typename Type>
	TransformMatrix& Move(const glm::vec<3, Type>& V)
	{
		return SetOffset(offset + glm::vec<3, TOffset>(GetOrientation() * V));
	}

	template<typename Type>
	TransformMatrix& Translate(const glm::vec<3, Type>& V)
	{
		return SetOffset(offset + V);
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

	const glm::mat<4, 4, TOffset>& GetMatrix() const
	{
		return matrix;
	}
};

using WorldMatrix = TransformMatrix<float, double>; 
using LocalMatrix = TransformMatrix<float, double>;