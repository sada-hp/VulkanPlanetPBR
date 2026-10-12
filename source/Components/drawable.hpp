#pragma once
#include "Math/PlanetoidMatrix.hpp"
#include "Materials/IMaterial.hpp"
#include "VulkanAPI/mesh.hpp"
#include "IComponent.hpp"

struct GDrawable : public IComponent
{
public:
	std::shared_ptr<IMaterial> Material = nullptr;
	std::shared_ptr<IMesh> Mesh = nullptr;

public:
	GDrawable() = default;

	GDrawable(std::shared_ptr<IMesh> InMesh, std::shared_ptr<IMaterial> InMaterial)
		: Material(InMaterial)
		, Mesh(InMesh)
	{

	}

	GDrawable(const GDrawable& other)
		: Material(other.Material)
		, Mesh(other.Mesh)
	{
	}
};