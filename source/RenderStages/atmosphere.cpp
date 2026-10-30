#include "atmosphere.hpp"
#include "Factories/VkSamplerFactory.hpp"

GAtmosphereStage::GAtmosphereStage(std::shared_ptr<RenderScope> Scope)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentLoadOp(GVkSharedResources::Instance.ColorBuffer->GetFormat(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	RenderPass = RPDesc.Construct(Scope);

	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector{ GVkSharedResources::Instance.ColorBuffer });

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, GVkBuffer::ToView(GVkSharedResources::Instance.UBO));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(GVkSharedResources::Instance.IrradianceLUT), GVkSamplerFactory::LinearSamplerClamp(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(GVkSharedResources::Instance.ScatteringLUT), GVkSamplerFactory::LinearSamplerClamp(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(GVkSharedResources::Instance.TransmittanceLUT), GVkSamplerFactory::LinearSamplerClamp(Scope));
	DescriptorSet = DSDesc.Allocate(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.VS.AppendCode(GShaders::FullscreenVS);
	PSODesc.PS.AppendCode(GShaderUtils::UBOCommon)
		.AppendCode(GShaderUtils::LightingCommon)
		.AppendCode(GShaders::AtmospherePS);
	PSODesc.AddDescriptorLayout(DescriptorSet->GetLayout());
	PSODesc.SetRenderPass(RenderPass);

	PSODesc.AttachmentBlendState(0).blendEnable = VK_TRUE;
	PSODesc.AttachmentBlendState(0).colorBlendOp = VK_BLEND_OP_ADD;
	PSODesc.AttachmentBlendState(0).srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	PSODesc.AttachmentBlendState(0).dstColorBlendFactor = VK_BLEND_FACTOR_ONE;

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