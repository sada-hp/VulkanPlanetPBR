#include "clouds.hpp"
#include "Factories/VkSamplerFactory.hpp"

GCloudsStage::GCloudsStage(std::shared_ptr<RenderScope> Scope, const GVkSharedResources& Resources)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentLoadOp(Resources.ColorBuffer->GetFormat(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	RenderPass = RPDesc.Construct(Scope);

	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector{ Resources.ColorBuffer });

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, GVkBuffer::ToView(Resources.UBO));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.IrradianceLUT), GVkSamplerFactory::LinearSamplerClamp(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.ScatteringLUT), GVkSamplerFactory::LinearSamplerClamp(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.TransmittanceLUT), GVkSamplerFactory::LinearSamplerClamp(Scope));
	DescriptorSet = DSDesc.Allocate(Scope);
}

GCloudsStage::~GCloudsStage()
{
}

void GCloudsStage::Execute(const GCamera& Camera, const IWorld& World)
{
	CommandBuffer->BeginRenderPass(RenderPass, Framebuffer);
	CommandBuffer->BindPipeline(Pipeline);
	CommandBuffer->BindDescriptorSet(0, DescriptorSet);
	CommandBuffer->Draw(3);
	CommandBuffer->Submit();
}