#pragma once
#include "IMaterial.hpp"

class GMeshMaterial : public IMaterial
{
public:
	GMeshMaterial(std::shared_ptr<RenderScope> Scope, const MaterialDescriptor& Descriptor);

	static std::shared_ptr<GMeshMaterial> Create(std::shared_ptr<RenderScope> Scope, const MaterialDescriptor& Descriptor = {})
	{
		return std::make_shared<GMeshMaterial>(Scope, Descriptor);
	}
};