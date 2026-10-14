#pragma once
#include "Math/PlanetoidMatrix.hpp"
#include "Materials/IMaterial.hpp"
#include "VulkanAPI/mesh.hpp"
#include "IComponent.hpp"

struct GDrawable : public IComponent
{
public:
	std::shared_ptr<IMesh> Mesh = nullptr;
	std::vector<std::shared_ptr<IMaterial>> Materials = {};

public:
	GDrawable() = default;

	GDrawable(std::shared_ptr<IMesh> InMesh, std::shared_ptr<IMaterial> InMaterial)
		: Materials({ InMaterial })
		, Mesh(InMesh)
	{

	}

	GDrawable(std::shared_ptr<IMesh> InMesh, std::vector<std::shared_ptr<IMaterial>> InMaterials)
		: Materials(InMaterials)
		, Mesh(InMesh)
	{

	}

	GDrawable(const GDrawable& other)
		: Materials(other.Materials)
		, Mesh(other.Mesh)
	{
	}

	virtual ~GDrawable()
	{
	}
};