#include "mesh_objects.hpp"
#include "Factories/VkImageFactory.hpp"
#include "Factories/VkSamplerFactory.hpp"

GMeshStage::GMeshStage(std::shared_ptr<RenderScope> Scope)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	Images.DeferredColor = GVkImageFactory::Image(Scope, VK_FORMAT_R16G16B16A16_SFLOAT, GVkSharedResources::Instance.ColorBuffer->GetExtent(), EImageFlags::RenderTarget | EImageFlags::SubpassInput | EImageFlags::InFlight);
	Images.DeferredNormal = GVkImageFactory::Image(Scope, VK_FORMAT_R16G16B16A16_SFLOAT, GVkSharedResources::Instance.ColorBuffer->GetExtent(), EImageFlags::RenderTarget | EImageFlags::SubpassInput | EImageFlags::InFlight);
	Images.DeferredPosition = GVkImageFactory::Image(Scope, VK_FORMAT_R32G32B32A32_SFLOAT, GVkSharedResources::Instance.ColorBuffer->GetExtent(), EImageFlags::RenderTarget | EImageFlags::SubpassInput | EImageFlags::InFlight);

	RenderPass = GetRenderPass(Scope);
	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector
	{ 
		GVkImage::ToView(Images.DeferredColor),
		GVkImage::ToView(Images.DeferredNormal),
		GVkImage::ToView(Images.DeferredPosition),
		GVkImage::ToView(GVkSharedResources::Instance.ColorBuffer),
		GVkImage::ToView(GVkSharedResources::Instance.DepthBuffer)
	});

	_initDrawPass();
	_initCompositePass();
}

GMeshStage::~GMeshStage()
{

}

void GMeshStage::_initDrawPass()
{
	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkBuffer::ToView(GVkSharedResources::Instance.UBO));
	Descriptors.Draw = DSDesc.Allocate(Scope);
}

void GMeshStage::_initCompositePass()
{
	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkBuffer::ToView(GVkSharedResources::Instance.UBO));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(GVkSharedResources::Instance.IrradianceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(GVkSharedResources::Instance.ScatteringLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(GVkSharedResources::Instance.TransmittanceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddSubpassAttachment(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Images.DeferredColor));
	DSDesc.AddSubpassAttachment(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Images.DeferredNormal));
	DSDesc.AddSubpassAttachment(VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Images.DeferredPosition));
	Descriptors.Composite = DSDesc.Allocate(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.VS.AppendCode(GShaders::FullscreenVS);
	PSODesc.PS.AppendCode(GShaderUtils::UBOCommon)
		.AppendCode(GShaderUtils::LightingCommon)
		.AppendCode(GShaders::MeshCompositePS);
	PSODesc.AddDescriptorLayout(Descriptors.Composite->GetLayout());
	PSODesc.SetRenderPass(RenderPass, 1);

	PSODesc.AttachmentBlendState(0).blendEnable = VK_TRUE;
	PSODesc.AttachmentBlendState(0).colorBlendOp = VK_BLEND_OP_ADD;
	PSODesc.AttachmentBlendState(0).srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	PSODesc.AttachmentBlendState(0).srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	PSODesc.AttachmentBlendState(0).dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	PSODesc.AttachmentBlendState(0).dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	Pipelines.Composite = PSODesc.Construct(Scope);
}

void GVkCommandBuffer::NextSubpass()
{
	auto& object = _activeObj();

	assert(object.renderPass);
	_nextSubpass();
}

void GMeshStage::Execute(const GCamera& Camera, const IWorld& World)
{
	auto view = World.GetRegistry().view<EllipsoidMatrix, GDrawable>();

	VkRect2D Scissor = Framebuffer->GetRenderArea();
	VkViewport Viewport = { 0, 0, Scissor.extent.width, Scissor.extent.height, 0.0, 1.0 };

	CommandBuffer->BeginRenderPass(RenderPass, Framebuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	std::vector<VkCommandBuffer> _commands;
	_commands.reserve(view.size_hint());

	for (const auto& [ent, matrix, object] : view.each())
	{
		VkCommandBuffer subCommand = object._getPrerecordedCommands(RenderPass, Framebuffer, matrix.GetBufferGPU(), Descriptors.Draw);
		_commands.emplace_back(subCommand);
	}

	CommandBuffer->Execute(_commands);

	CommandBuffer->NextSubpass();

	CommandBuffer->SetRenderArea(Scissor, Viewport);

	CommandBuffer->BindPipeline(Pipelines.Composite);
	CommandBuffer->BindDescriptorSet(0, Descriptors.Composite);
	CommandBuffer->Draw(3);

	CommandBuffer->Submit();
}