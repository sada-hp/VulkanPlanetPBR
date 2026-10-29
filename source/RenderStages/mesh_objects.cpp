#include "mesh_objects.hpp"
#include "Factories/VkImageFactory.hpp"
#include "Factories/VkSamplerFactory.hpp"

GMeshStage::GMeshStage(std::shared_ptr<RenderScope> Scope, const GVkSharedResources& Resources)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	Images.DeferredColor = GVkImageFactory::Image(Scope, VK_FORMAT_R16G16B16A16_SFLOAT, Resources.ColorBuffer->GetExtent(), EImageFlags::RenderTarget | EImageFlags::SubpassInput | EImageFlags::InFlight);
	Images.DeferredNormal = GVkImageFactory::Image(Scope, VK_FORMAT_R16G16B16A16_SFLOAT, Resources.ColorBuffer->GetExtent(), EImageFlags::RenderTarget | EImageFlags::SubpassInput | EImageFlags::InFlight);
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

	glm::dvec3 CameraOffset = Camera.GetWorldMatrix().GetPosition();
	for (auto& [object, matrix] : objects)
	{
		/*
		* Preprocess world matrix on CPU to avoid FP64 math on GPU
		*/
		glm::dmat4 WorldMatrix = matrix->GetMatrix();
		WorldMatrix[3].x -= CameraOffset.x;
		WorldMatrix[3].y -= CameraOffset.y;
		WorldMatrix[3].z -= CameraOffset.z;

		glm::mat4 CompressedMatrix = WorldMatrix;
		bool IsIndexed = object->Mesh->IndexCount() > 0;

		CommandBuffer->BindVertexBuffer(0, GVkBuffer::ToView(object->Mesh->VertexBuffer()));

		if (IsIndexed)
			CommandBuffer->BindIndexBuffer(object->Mesh->IndexType(), GVkBuffer::ToView(object->Mesh->IndexBuffer()));

		for (auto& material : object->Materials)
		{
			CommandBuffer->BindPipeline(material->GetPipeline());
			CommandBuffer->BindDescriptorSet(0, Descriptors.Draw);

			if (auto descriptorset = material->GetDescriptorSet())
				CommandBuffer->BindDescriptorSet(1, descriptorset);

			CommandBuffer->PushConstants(0, CompressedMatrix);

			if (IsIndexed)
				CommandBuffer->DrawIndexed(object->Mesh->IndexCount());
			else
				CommandBuffer->Draw(object->Mesh->VertexCount());
		}
	}

	CommandBuffer->BindPipeline(Pipelines.Composite);
	CommandBuffer->BindDescriptorSet(0, Descriptors.Composite);
	CommandBuffer->Draw(3);

	CommandBuffer->Submit();
}