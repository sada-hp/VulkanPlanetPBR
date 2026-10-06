#pragma once
#include "Math/ProjectionMatrix.hpp"
#include "Math/PlanetoidMatrix.hpp"

class GCamera
{
protected:
	EllipsoidMatrix m_EllipsTM;
	ProjectionMatrix m_ProjTM;

public:
	GCamera();
	~GCamera();

	glm::dmat4 ViewMat4() const;
	glm::dmat4 WorldMat4() const;
	glm::mat4 ProjectionMat4() const;
	glm::dmat4 ProjectionViewMat4() const;
	glm::dmat4 ProjectionViewInverseMat4() const;

	LocalMatrix& GetLocalMatrix();
	EllipsoidMatrix& GetWorldMatrix();
	ProjectionMatrix& GetProjectionMatrix();

	const LocalMatrix& GetLocalMatrix() const;
	const EllipsoidMatrix& GetWorldMatrix() const;
	const ProjectionMatrix& GetProjectionMatrix() const;
};