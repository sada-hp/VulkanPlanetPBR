#pragma once
#include "VulkanAPI/descriptor_set.hpp"
#include "VulkanAPI/pipeline.hpp"
#include "VulkanAPI/scope.hpp"

class IMaterial
{
protected:
	std::shared_ptr<RenderScope> Scope;
	std::shared_ptr<GVkPipeline> Pipeline;

public:
	IMaterial(std::shared_ptr<RenderScope> InScope)
		: Scope(InScope)
	{
	}

	virtual ~IMaterial()
	{
	}

	std::shared_ptr<GVkPipeline> GetPipeline() const { return Pipeline; }
};