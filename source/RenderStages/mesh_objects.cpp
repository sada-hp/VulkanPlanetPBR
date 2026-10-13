#include "mesh_objects.hpp"
#include "Factories/VkImageFactory.hpp"
#include "Factories/VkSamplerFactory.hpp"

GMeshStage::GMeshStage(std::shared_ptr<RenderScope> Scope, const GVkSharedResources& Resources)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	Images.DeferredColor = GVkImageFactory::Image(Scope, VK_FORMAT_R32G32B32A32_SFLOAT, Resources.ColorBuffer->GetExtent(), EImageFlags::RenderTarget | EImageFlags::SubpassInput | EImageFlags::InFlight);
	Images.DeferredNormal = GVkImageFactory::Image(Scope, VK_FORMAT_R32G32B32A32_SFLOAT, Resources.ColorBuffer->GetExtent(), EImageFlags::RenderTarget | EImageFlags::SubpassInput | EImageFlags::InFlight);
	Images.DeferredPosition = GVkImageFactory::Image(Scope, VK_FORMAT_R32G32B32A32_SFLOAT, Resources.ColorBuffer->GetExtent(), EImageFlags::RenderTarget | EImageFlags::SubpassInput | EImageFlags::InFlight);

	RenderPass = GetRenderPass(Scope);
	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector
	{ 
		GVkImage::ToView(Images.DeferredColor),
		GVkImage::ToView(Images.DeferredNormal),
		GVkImage::ToView(Images.DeferredPosition),
		GVkImage::ToView(Resources.ColorBuffer), 
		GVkImage::ToView(Resources.DepthBuffer) 
	});

	_initDrawPass(Resources);
	_initCompositePass(Resources);
}

GMeshStage::~GMeshStage()
{

}

void GMeshStage::_initDrawPass(const GVkSharedResources& Resources)
{
	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkBuffer::ToView(Resources.UBO));
	Descriptors.Draw = DSDesc.Allocate(Scope);
}

void GMeshStage::_initCompositePass(const GVkSharedResources& Resources)
{
	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkBuffer::ToView(Resources.UBO));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.IrradianceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.ScatteringLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.TransmittanceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
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

void GMeshStage::Execute(const GCamera& Camera, const IWorld& World)
{
	CommandBuffer->BeginRenderPass(RenderPass, Framebuffer);

	std::vector<std::pair<const GDrawable*, const EllipsoidMatrix*>> objects;
	World.GetDrawableObjects(objects);

	for (auto& [object, matrix] : objects)
	{
		CommandBuffer->BindPipeline(object->Material->GetPipeline());
		CommandBuffer->BindDescriptorSet(0, Descriptors.Draw);

		CommandBuffer->BindVertexBuffer(0, GVkBuffer::ToView(object->Mesh->VertexBuffer()));

		glm::dmat4 WorldMatrix = matrix->GetMatrix<double>();
		CommandBuffer->PushConstants(0, WorldMatrix);

		if (object->Material->GetDescriptorSet())
			CommandBuffer->BindDescriptorSet(1, object->Material->GetDescriptorSet());

		if (object->Mesh->IndexCount() > 0)
		{
			CommandBuffer->BindIndexBuffer(object->Mesh->IndexType(), GVkBuffer::ToView(object->Mesh->IndexBuffer()));
			CommandBuffer->DrawIndexed(object->Mesh->IndexCount());
		}
		else
		{
			CommandBuffer->Draw(object->Mesh->VertexCount());
		}
	}

	CommandBuffer->BindPipeline(Pipelines.Composite);
	CommandBuffer->BindDescriptorSet(0, Descriptors.Composite);
	CommandBuffer->Draw(3);

	CommandBuffer->Submit();
}