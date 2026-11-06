#include "clear_pass.hpp"

void GAttachmentsClear::setup_dependencies(const IWorld& World)
{
	auto color_view = GVkImage::ToView(GVkSharedResources::Instance.ColorBuffer);
	Resources.add(color_view, EResourceOp::WriteOnly, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT);

	auto depth_view = GVkImage::ToView(GVkSharedResources::Instance.DepthBuffer);
	Resources.add(depth_view, EResourceOp::WriteOnly, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
}

GAttachmentsClear::GAttachmentsClear(std::shared_ptr<RenderScope> Scope)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	RenderPassDescriptor RPDesc;
	RPDesc.AddAttachmentClearOp(GVkSharedResources::Instance.ColorBuffer->GetFormat(), VkClearValue{}, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	RPDesc.AddAttachmentClearOp(GVkSharedResources::Instance.DepthBuffer->GetFormat(), VkClearValue{}, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	RenderPass = RPDesc.Construct(Scope);

	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector{
		GVkImage::ToView(GVkSharedResources::Instance.ColorBuffer),
		GVkImage::ToView(GVkSharedResources::Instance.DepthBuffer)
	});
}

GAttachmentsClear::~GAttachmentsClear()
{
}

void GAttachmentsClear::record(const GCamera& Camera, const IWorld& World)
{
	CommandBuffer->BeginRenderPass(RenderPass, Framebuffer);
	CommandBuffer->EndRenderPass();
}