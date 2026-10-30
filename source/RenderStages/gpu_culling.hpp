#pragma once
#include "VulkanAPI/shared_resources.hpp"
#include "VulkanAPI/descriptor_set.hpp"
#include "VulkanAPI/scope.hpp"
#include "IRenderStage.hpp"

class GCullingStage : public IRenderStage
{
	struct _shaderData
	{
		glm::vec4 planes[6];
		uint32_t invocations;
	};

	std::shared_ptr<GVkDescriptorSet> DescriptorSet = VK_NULL_HANDLE;
	std::shared_ptr<GVkPipeline> Pipeline = VK_NULL_HANDLE;

public:
	GCullingStage(std::shared_ptr<RenderScope> Scope);
	virtual ~GCullingStage();

	void Execute(const GCamera& Camera, const IWorld& World) override;
};