#pragma once
#include "VulkanAPI/shared_resources.hpp"
#include "VulkanAPI/descriptor_set.hpp"
#include "VulkanAPI/scope.hpp"
#include "IRenderStage.hpp"

class GMeshStage : public IRenderStage
{
private:
	struct
	{
		std::shared_ptr<GVkImage> DeferredColor = VK_NULL_HANDLE;
		std::shared_ptr<GVkImage> DeferredNormal = VK_NULL_HANDLE;
		std::shared_ptr<GVkImage> DeferredPosition = VK_NULL_HANDLE;
	} Images;

	struct
	{
		std::shared_ptr<GVkPipeline> Composite = VK_NULL_HANDLE;
	} Pipelines;

	struct
	{
		std::shared_ptr<GVkDescriptorSet> Draw = VK_NULL_HANDLE;
		std::shared_ptr<GVkDescriptorSet> Composite = VK_NULL_HANDLE;
	} Descriptors;

private:
	void _initDrawPass(const GVkSharedResources& Resources);
	void _initCompositePass(const GVkSharedResources& Resources);

public:
	GMeshStage(std::shared_ptr<RenderScope> Scope, const GVkSharedResources& Resources);
	virtual ~GMeshStage();

	void Execute(const GCamera& Camera, const IWorld& World) override;

	inline static std::shared_ptr<GVkRenderPass> GetRenderPass(std::shared_ptr<RenderScope> Scope)
	{
		RenderPassDescriptor RPDesc{};
		/* 0 */ RPDesc.AddAttachmentClearOp(VK_FORMAT_R16G16B16A16_SFLOAT);
		/* 1 */ RPDesc.AddAttachmentClearOp(VK_FORMAT_R16G16B16A16_SFLOAT);
		/* 2 */ RPDesc.AddAttachmentClearOp(VK_FORMAT_R32G32B32A32_SFLOAT);
		/* 3 */ RPDesc.AddAttachmentLoadOp(RenderScope::GetColorFormat(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		/* 4 */ RPDesc.AddAttachmentLoadOp(RenderScope::GetDepthFormat(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		RPDesc.AddSubpass(GVkSubpassDescription{ .colorAttachmentsIndices = {0, 1, 2}, .depthAttachmentIndex = 4 });
		RPDesc.AddSubpass(GVkSubpassDescription{ .colorAttachmentsIndices = {3}, .inputAttachmentsIndices = {0,1,2} });

		return RPDesc.Construct(Scope);
	}
};