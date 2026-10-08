#include "mesh_objects.hpp"
#include "Factories/VkSamplerFactory.hpp"

GMeshStage::GMeshStage(std::shared_ptr<RenderScope> Scope, const GVkSharedResources& Resources)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentLoadOp(RenderScope::GetColorFormat(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	RPDesc.AddAttachmentClearOp(RenderScope::GetDepthFormat(), VkClearValue{ .depthStencil = { 0.f, 0 } });
	RenderPass = RPDesc.Construct(Scope);

	Framebuffer = std::make_shared<GVkFramebuffer>(Scope, RenderPass, std::vector{ GVkImage::ToView(Resources.ColorBuffer), GVkImage::ToView(Resources.DepthBuffer) });

	DescriptorSetDescriptor DSDesc{};
	DSDesc.AddUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkBuffer::ToView(Resources.UBO));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.IrradianceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.ScatteringLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DSDesc.AddImageSampler(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, GVkImage::ToView(Resources.TransmittanceLUT), GVkSamplerFactory::LinearSamplerRepeat(Scope));
	DescriptorSet = DSDesc.Allocate(Scope);
}

GMeshStage::~GMeshStage()
{

}

void GMeshStage::Execute(const GCamera& Camera, const IWorld& World)
{
	CommandBuffer->BeginRenderPass(RenderPass, Framebuffer);

	auto objects = World.GetDrawableObjects();
	for (auto& object : objects)
	{
		CommandBuffer->BindPipeline(object.Material->GetPipeline());
		CommandBuffer->BindDescriptorSet(0, DescriptorSet);

		CommandBuffer->BindVertexBuffer(0, GVkBuffer::ToView(object.Mesh->VertexBuffer()));

		glm::dmat4 WorldMatrix = object.WorldMatrix.GetMatrix<double>();
		CommandBuffer->PushConstants(0, WorldMatrix);

		if (object.Material->GetDescriptorSet())
			CommandBuffer->BindDescriptorSet(1, object.Material->GetDescriptorSet());

		if (object.Mesh->IndexCount() > 0)
		{
			CommandBuffer->BindIndexBuffer(object.Mesh->IndexType(), GVkBuffer::ToView(object.Mesh->IndexBuffer()));
			CommandBuffer->DrawIndexed(object.Mesh->IndexCount());
		}
		else
		{
			CommandBuffer->Draw(object.Mesh->VertexCount());
		}
	}

	CommandBuffer->Submit();
}