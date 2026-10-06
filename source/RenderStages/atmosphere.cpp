#include "atmosphere.hpp"
#include "Factories/VkSamplerFactory.hpp"

GAtmosphereStage::GAtmosphereStage(std::shared_ptr<RenderScope> Scope, const GVkSharedResources& Resources)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentDontCareOp(Resources.ColorBuffer->GetFormat(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	RenderPass = RPDesc.Construct(Scope);

	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector{ Resources.ColorBuffer });

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, GVkBuffer::ToView(Resources.UBO));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.IrradianceLUT), GVkSamplerFactory::LinearSamplerClamp(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.ScatteringLUT), GVkSamplerFactory::LinearSamplerClamp(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.TransmittanceLUT), GVkSamplerFactory::LinearSamplerClamp(Scope));
	DescriptorSet = DSDesc.Allocate(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.VS.AppendCode(GShaders::FullscreenVS);
	PSODesc.PS.AppendCode(GShaderUtils::UBOCommon)
		.AppendCode(GShaderUtils::LightingCommon)
		.AppendCode(GShaders::AtmospherePS);
	PSODesc.AddDescriptorLayout(DescriptorSet->GetLayout());
	PSODesc.SetRenderPass(RenderPass);
	Pipeline = PSODesc.Construct(Scope);
}

GAtmosphereStage::~GAtmosphereStage()
{
}

void GAtmosphereStage::Execute(const GCamera& Camera, const IWorld& World)
{
	CommandBuffer->BeginRenderPass(RenderPass, Framebuffer);
	CommandBuffer->BindPipeline(Pipeline);
	CommandBuffer->BindDescriptorSet(0, DescriptorSet);
	CommandBuffer->Draw(3);
	CommandBuffer->Submit();
}