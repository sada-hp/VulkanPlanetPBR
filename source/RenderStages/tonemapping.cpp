#include "tonemapping.hpp"
#include "Factories/VkSamplerFactory.hpp"

void GTonemapStage::setup_dependencies(const IWorld& World)
{
	auto color_view = GVkImage::ToView(GVkSharedResources::Instance.ColorBuffer);
	Resources.add(color_view, EResourceOp::ReadOnly, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);

	auto final_view = GVkImage::ToView(GVkSharedResources::Instance.FinalTarget);
	Resources.add(final_view, EResourceOp::WriteOnly, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT);
}

GTonemapStage::GTonemapStage(std::shared_ptr<RenderScope> InScope)
	: IRenderStage(InScope, VK_QUEUE_GRAPHICS_BIT)
{
	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentDontCareOp(GVkSharedResources::Instance.FinalTarget->GetFormat(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	RenderPass = RPDesc.Construct(Scope);

	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector{ GVkImage::ToView(GVkSharedResources::Instance.FinalTarget) });

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(GVkSharedResources::Instance.ColorBuffer), GVkSamplerFactory::PointSamplerClamp(Scope));
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

void GTonemapStage::record(const GCamera& Camera, const IWorld& World)
{
	CommandBuffer->BeginRenderPass(RenderPass, Framebuffer);
	CommandBuffer->BindPipeline(Pipeline);
	CommandBuffer->BindDescriptorSet(0, DescriptorSet);
	CommandBuffer->Draw(3);
}