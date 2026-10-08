#pragma once
#include "VulkanAPI/shared_resources.hpp"
#include "VulkanAPI/descriptor_set.hpp"
#include "VulkanAPI/scope.hpp"
#include "IRenderStage.hpp"

class GTonemapStage : public IRenderStage
{
private:
	std::shared_ptr<GVkPipeline> Pipeline = VK_NULL_HANDLE;
	std::shared_ptr<GVkDescriptorSet> DescriptorSet = VK_NULL_HANDLE;

public:
	GTonemapStage(std::shared_ptr<RenderScope> Scope, const GVkSharedResources& Resources);
	virtual ~GTonemapStage();
	void Execute(const GCamera& Camera, const IWorld& World) override;
};