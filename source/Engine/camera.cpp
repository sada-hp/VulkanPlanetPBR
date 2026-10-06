#include "camera.hpp"
#include "Math/EllipsoidMath.hpp"

GCamera::GCamera()
{
	GetProjectionMatrix().SetFOV(45.f)
		.SetDepthRange(0.01, 1e9);
}

GCamera::~GCamera()
{
}

glm::dmat4 GCamera::ViewMat4() const
{
	auto eye = m_EllipsTM.GetPosition();
	auto fwd = m_EllipsTM.GetForward();
	auto up  = m_EllipsTM.GetUp();

	return glm::lookAt(eye, eye + fwd, up);
}

glm::dmat4 GCamera::WorldMat4() const
{
	return m_EllipsTM.GetMatrix();
}

glm::mat4 GCamera::ProjectionMat4() const
{
	return m_ProjTM.GetMatrix();
}

glm::dmat4 GCamera::ProjectionViewMat4() const
{
	return glm::dmat4(ProjectionMat4()) * ViewMat4();
}

glm::dmat4 GCamera::ProjectionViewInverseMat4() const
{
	return glm::inverse(ViewMat4()) * glm::inverse(glm::dmat4(ProjectionMat4()));
}

EllipsoidMatrix& GCamera::GetWorldMatrix()
{
	return m_EllipsTM;
}

WorldMatrix& GCamera::GetLocalMatrix()
{
	return m_EllipsTM.GetLocalMatrix();
}

ProjectionMatrix& GCamera::GetProjectionMatrix()
{
	return m_ProjTM;
}

const EllipsoidMatrix& GCamera::GetWorldMatrix() const
{
	return m_EllipsTM;
}

const WorldMatrix& GCamera::GetLocalMatrix() const
{
	return m_EllipsTM.GetLocalMatrix();
}

const ProjectionMatrix& GCamera::GetProjectionMatrix() const
{
	return m_ProjTM;
}