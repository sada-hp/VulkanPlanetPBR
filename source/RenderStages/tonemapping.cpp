#include "tonemapping.hpp"
#include "Factories/VkSamplerFactory.hpp"

GTonemapStage::GTonemapStage(std::shared_ptr<RenderScope> InScope, const GVkSharedResources& Resources)
	: IRenderStage(InScope, VK_QUEUE_GRAPHICS_BIT)
{
	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentDontCareOp(Resources.FinalTarget->GetFormat());
	RenderPass = RPDesc.Construct(Scope);

	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector{ GVkImage::ToView(Resources.FinalTarget) });

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.ColorBuffer), GVkSamplerFactory::PointSamplerClamp(Scope));
	DescriptorSet = DSDesc.Allocate(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.AddDescriptorLayout(DescriptorSet->GetLayout());
	PSODesc.VS.AppendCode(GShaders::FullscreenVS);
	PSODesc.PS.AppendCode(GShaders::TonemapPS);
	PSODesc.SetRenderPass(RenderPass);
	Pipeline = PSODesc.Construct(Scope);
}

GTonemapStage::~GTonemapStage()
{
}

void GTonemapStage::Execute(const GCamera& Camera, const IWorld& World)
{
	CommandBuffer->BeginRenderPass(RenderPass, Framebuffer);
	CommandBuffer->BindPipeline(Pipeline);
	CommandBuffer->BindDescriptorSet(0, DescriptorSet);
	CommandBuffer->Draw(3);
	CommandBuffer->Submit();
}