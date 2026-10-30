#include "clear_pass.hpp"

GAttachmentsClear::GAttachmentsClear(std::shared_ptr<RenderScope> Scope)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	RenderPassDescriptor RPDesc;
	RPDesc.AddAttachmentClearOp(GVkSharedResources::Instance.ColorBuffer->GetFormat());
	RPDesc.AddAttachmentClearOp(GVkSharedResources::Instance.DepthBuffer->GetFormat());
	RenderPass = RPDesc.Construct(Scope);

	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector{
		GVkImage::ToView(GVkSharedResources::Instance.ColorBuffer),
		GVkImage::ToView(GVkSharedResources::Instance.DepthBuffer)
	});
}

GAttachmentsClear::~GAttachmentsClear()
{
}

void GAttachmentsClear::Execute(const GCamera& Camera, const IWorld& World)
{
	CommandBuffer->BeginRenderPass(RenderPass, Framebuffer);
	CommandBuffer->Submit();
}