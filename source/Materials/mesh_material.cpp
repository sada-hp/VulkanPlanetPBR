#include "VulkanAPI/vertex.hpp"
#include "Factories/VkSamplerFactory.hpp"
#include "mesh_material.hpp"
#include "glm/glm.hpp"

GMeshMaterial::GMeshMaterial(std::shared_ptr<RenderScope> Scope, const MaterialDescriptor& Descriptor)
	: IMaterial(Scope)
{
	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentLoadOp(RenderScope::GetColorFormat(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	RPDesc.AddAttachmentClearOp(RenderScope::GetDepthFormat(), VkClearValue{});
	std::shared_ptr<GVkRenderPass> RenderPass = RPDesc.Construct(Scope);

	DescriptorLayoutDescriptor DSLDesc{};
	DSLDesc.AddUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	DSLDesc.AddImageSampler(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	DSLDesc.AddImageSampler(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	DSLDesc.AddImageSampler(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	std::shared_ptr<GVkDescriptorSetLayout> Layout = DSLDesc.Construct(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.AddPushConstants<glm::dmat4>(VK_SHADER_STAGE_VERTEX_BIT);
	PSODesc.AddDescriptorLayout(Layout->GetLayout());
	PSODesc.SetRenderPass(RenderPass);
	PSODesc.SetVertexAttributeBindings(MeshVertex::getAttributeDescriptions())
		.SetVertexInputBindings(MeshVertex::getBindingDescriptions())
		.SetCullMode(Descriptor.CullMode, Descriptor.FrontFace)
		.SetPrimitiveTopology(Descriptor.PrimitiveTopology)
		.SetPolygonMode(Descriptor.PolygonMode);

	PSODesc.VS.AppendCode(GShaderUtils::UBOCommon)
		.AppendCode(GShaderUtils::LightingCommon)
		.AppendCode(GShaders::MeshVS);

	PSODesc.PS.AppendCode(GShaderUtils::UBOCommon)
		.AppendCode(GShaderUtils::LightingCommon);

	if (Descriptor.SubmeshTextures.empty())
	{
		PSODesc.PS.AppendCode(GShaders::MeshDefaultPS);
	}
	else
	{
		PSODesc.PS.AppendCode(GShaders::MeshPS);

		std::vector<std::pair<std::shared_ptr<GVkImageView>, std::shared_ptr<GVkSampler>>> albedoResources;
		for (const auto& pack : Descriptor.SubmeshTextures)
		{
			std::shared_ptr<GVkSampler> albedoSampler = GVkSamplerFactory::LinearSamplerRepeat(Scope, pack.Albedo->GetMipLevelsCount());
			albedoResources.emplace_back(GVkImage::ToView(pack.Albedo), albedoSampler);
		}

		DescriptorSetDescriptor DSDesc{};
		DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, albedoResources);
		DescriptorSet = DSDesc.Allocate(Scope);

		PSODesc.AddDescriptorLayout(DescriptorSet->GetLayout());
	}

	Pipeline = PSODesc.Construct(Scope);
}