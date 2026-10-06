#include "VulkanAPI/vertex.hpp"
#include "mesh_material.hpp"
#include "glm/glm.hpp"

GMeshMaterial::GMeshMaterial(std::shared_ptr<RenderScope> Scope, VkCullModeFlagBits CullMode, VkPrimitiveTopology Topology)
	: IMaterial(Scope)
{
	RenderPassDescriptor RPDesc{};
	RPDesc.AddAttachmentLoadOp(RenderScope::GetColorFormat(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	std::shared_ptr<GVkRenderPass> RenderPass = RPDesc.Construct(Scope);

	DescriptorLayoutDescriptor DSLDesc{};
	DSLDesc.AddUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT);
	std::shared_ptr<GVkDescriptorSetLayout> Layout = DSLDesc.Construct(Scope);

	GraphicsPipelineDescriptor PSODesc{};
	PSODesc.AddPushConstants<glm::dmat4>(VK_SHADER_STAGE_VERTEX_BIT);
	PSODesc.VS.AppendCode(GShaderUtils::UBOCommon)
		.AppendCode(GShaders::MeshVS);
	PSODesc.PS.AppendCode(GShaders::MeshPS);
	PSODesc.AddDescriptorLayout(Layout->GetLayout());
	PSODesc.SetRenderPass(RenderPass);
	PSODesc.SetVertexAttributeBindings(MeshVertex::getAttributeDescriptions())
		.SetVertexInputBindings(MeshVertex::getBindingDescriptions())
		.SetPrimitiveTopology(Topology)
		.SetCullMode(CullMode);

	Pipeline = PSODesc.Construct(Scope);
}