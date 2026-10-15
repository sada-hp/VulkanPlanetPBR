#include "clear_pass.hpp"

GAttachmentsClear::GAttachmentsClear(std::shared_ptr<RenderScope> Scope, const GVkSharedResources& Resources)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	RenderPassDescriptor RPDesc;
	RPDesc.AddAttachmentClearOp(Resources.ColorBuffer->GetFormat());
	RPDesc.AddAttachmentClearOp(Resources.DepthBuffer->GetFormat());
	RenderPass = RPDesc.Construct(Scope);

	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector{
		GVkImage::ToView(Resources.ColorBuffer),
		GVkImage::ToView(Resources.DepthBuffer)
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