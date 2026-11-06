#include "mesh_objects.hpp"
#include "Factories/VkImageFactory.hpp"
#include "Factories/VkSamplerFactory.hpp"

void GMeshStage::setup_dependencies(const IWorld& World)
{
	auto color_view = GVkImage::ToView(GVkSharedResources::Instance.ColorBuffer);
	Resources.add(color_view, EResourceOp::ReadWrite, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT);

	auto depth_view = GVkImage::ToView(GVkSharedResources::Instance.DepthBuffer);
	Resources.add(depth_view, EResourceOp::ReadWrite, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT);

	auto indirect_view = World.GetSceneIndirectBuffer();
	Resources.add(indirect_view, EResourceOp::ReadOnly, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT);

	auto transform_view = World.GetSceneTransformBuffer();
	Resources.add(transform_view, EResourceOp::ReadOnly, VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT);

	auto ubo_view = GVkBuffer::ToView(GVkSharedResources::Instance.UBO);
	Resources.add(ubo_view, EResourceOp::ReadOnly, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);

	Commands.Draw = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_GRAPHICS_BIT, ECmdFlags::InFlight | ECmdFlags::Secondary);
}

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

bool GMeshStage::IsActive(const IWorld& World)
{
	auto storage = World.GetRegistry().storage<GDrawable>();

	if (storage && !storage->empty())
		return true;

	return false;
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

void GMeshStage::_exeuteSecondary(const IWorld& World)
{
	if (!Commands.Draw->IsRecorded())
	{
		auto view = World.GetRegistry().view<EllipsoidMatrix, GDrawable>();
		
		Commands.Draw->BeginRenderPass(RenderPass, Framebuffer);
		for (const auto& [ent, matrix, draw] : view.each())
		{
			Commands.Draw->BeginConditionalRendering(draw._conditionalView);
			{
				Commands.Draw->BindVertexBuffer(0, draw.Mesh->VertexBuffer());
				Commands.Draw->BindVertexBuffer(1, draw._transformView);

				bool IsIndexed = draw.Mesh->IndexCount() > 0;

				if (IsIndexed)
					Commands.Draw->BindIndexBuffer(draw.Mesh->IndexType(), draw.Mesh->IndexBuffer());

				for (auto& material : draw.Materials)
				{
					Commands.Draw->BindPipeline(material->GetPipeline());
					Commands.Draw->BindDescriptorSet(0, Descriptors.Draw);

					if (auto descriptorset = material->GetDescriptorSet())
						Commands.Draw->BindDescriptorSet(1, descriptorset);

					if (IsIndexed)
						Commands.Draw->DrawIndirectIndexed(draw._indirectCallView);
					else
						Commands.Draw->DrawIndirect(draw._indirectCallView);
				}
			}
			Commands.Draw->EndConditionalRendering();
		}

		Commands.Draw->Submit();
	}

	CommandBuffer->Execute(Commands.Draw);
}

void GMeshStage::record(const GCamera& Camera, const IWorld& World)
{
	auto view = World.GetRegistry().view<EllipsoidMatrix, GDrawable>();

	VkRect2D Scissor = Framebuffer->GetRenderArea();
	VkViewport Viewport = { 0, 0, Scissor.extent.width, Scissor.extent.height, 0.0, 1.0 };

	CommandBuffer->BeginRenderPass(RenderPass, Framebuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	_exeuteSecondary(World);

	CommandBuffer->NextSubpass(VK_SUBPASS_CONTENTS_INLINE);

	CommandBuffer->SetRenderArea(Scissor, Viewport);

	CommandBuffer->BindPipeline(Pipelines.Composite);
	CommandBuffer->BindDescriptorSet(0, Descriptors.Composite);
	CommandBuffer->Draw(3);

	CommandBuffer->EndRenderPass();
}