#pragma once
#include "Math/PlanetoidMatrix.hpp"
#include "Materials/IMaterial.hpp"
#include "VulkanAPI/mesh.hpp"
#include "IComponent.hpp"

struct GDrawable : public IComponent
{
	std::shared_ptr<IMaterial> Material = nullptr;
	std::shared_ptr<IMesh> Mesh = nullptr;
	EllipsoidMatrix WorldMatrix;
};