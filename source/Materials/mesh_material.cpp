#include "VulkanAPI/vertex.hpp"
#include "Factories/VkSamplerFactory.hpp"
#include "RenderStages/mesh_objects.hpp"
#include "mesh_material.hpp"
#include "glm/glm.hpp"

GMeshMaterial::GMeshMaterial(std::shared_ptr<RenderScope> Scope, const MaterialDescriptor& Descriptor)
	: IMaterial(Scope)
{
	std::shared_ptr<GVkRenderPass> RenderPass = GMeshStage::GetRenderPass(Scope);

	DescriptorLayoutDescriptor DSLDesc{};
	DSLDesc.AddUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	std::shared_ptr<GVkDescriptorSetLayout> Layout = DSLDesc.Construct(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.AddDescriptorLayout(Layout->GetLayout());
	PSODesc.SetRenderPass(RenderPass);
	PSODesc.SetCullMode(Descriptor.CullMode, Descriptor.FrontFace)
		.SetPrimitiveTopology(Descriptor.PrimitiveTopology)
		.SetPolygonMode(Descriptor.PolygonMode);

	std::vector<std::pair<std::shared_ptr<GVkImageView>, std::shared_ptr<GVkSampler>>> albedoResources;
	for (const auto& pack : Descriptor.SubmeshTextures)
	{
		std::shared_ptr<GVkSampler> albedoSampler = GVkSamplerFactory::LinearSamplerRepeat(Scope, pack.Albedo->GetMipLevelsCount());
		albedoResources.emplace_back(GVkImage::ToView(pack.Albedo), albedoSampler);
	}

	PSODesc.VS.AppendCode(GShaderUtils::UBOCommon)
		.AppendCode(GShaderUtils::LightingCommon);

	PSODesc.PS.AppendCode(GShaderUtils::UBOCommon)
		.AppendCode(GShaderUtils::LightingCommon);

	if (!albedoResources.empty())
	{
		DescriptorSetDescriptor DSDesc{};
		DSDesc.AddImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, albedoResources);
		DescriptorSet = DSDesc.Allocate(Scope);

		PSODesc.AddDescriptorLayout(DescriptorSet->GetLayout());
		PSODesc.PS.AppendCode(GShaders::MeshPS);
	}
	else
	{
		PSODesc.PS.AppendCode(GShaders::MeshDefaultPS);
	}

	if (Descriptor.Compressed)
	{
		PSODesc.VS.AppendCode(GShaders::CompressedMeshVS);

		PSODesc.SetVertexAttributeBindings(CompressedVertex::getAttributeDescriptions())
			.SetVertexInputBindings(CompressedVertex::getBindingDescriptions());
	}
	else
	{
		PSODesc.VS.AppendCode(GShaders::MeshVS);

		PSODesc.SetVertexAttributeBindings(MeshVertex::getAttributeDescriptions())
			.SetVertexInputBindings(MeshVertex::getBindingDescriptions());
	}

	Pipeline = PSODesc.Construct(Scope);
}