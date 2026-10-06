#pragma once
#include "VulkanAPI/shared_resources.hpp"
#include "VulkanAPI/descriptor_set.hpp"
#include "VulkanAPI/scope.hpp"
#include "IRenderStage.hpp"

class GMeshStage : public IRenderStage
{
private:
	std::shared_ptr<GVkDescriptorSet> DescriptorSet = VK_NULL_HANDLE;

public:
	GMeshStage(std::shared_ptr<RenderScope> Scope, const GVkSharedResources& Resources);
	virtual ~GMeshStage();

	void Execute(const GCamera& Camera, const IWorld& World) override;
};