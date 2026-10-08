#pragma once
#include "VulkanAPI/shared_resources.hpp"
#include "VulkanAPI/descriptor_set.hpp"
#include "VulkanAPI/scope.hpp"
#include "IRenderStage.hpp"

class GCloudsStage : public IRenderStage
{
	struct
	{
		std::shared_ptr<GVkImage> PerlinWorley = VK_NULL_HANDLE;
		std::shared_ptr<GVkImage> HighFrequency = VK_NULL_HANDLE;
	} Images;

private:
	std::shared_ptr<GVkPipeline> Pipeline = VK_NULL_HANDLE;
	std::shared_ptr<GVkDescriptorSet> DescriptorSet = VK_NULL_HANDLE;

protected:
	std::shared_ptr<GVkImage> _generate_noise(const std::string& shader, VkFormat Format, VkExtent3D extents, uint32_t freq, uint32_t octaves);

public:
	GCloudsStage(std::shared_ptr<RenderScope> Scope, const GVkSharedResources& Resources);
	virtual ~GCloudsStage();
	void Execute(const GCamera& Camera, const IWorld& World) override;
};