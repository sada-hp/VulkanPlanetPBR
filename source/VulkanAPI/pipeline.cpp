#include "pch.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"

GVkPipeline::GVkPipeline(std::shared_ptr<RenderScope> InScope, VkPipelineLayoutCreateInfo LayoutInfo, VkGraphicsPipelineCreateInfo CreateInfo, VkPipelineCache Cache)
	: Scope(InScope)
	, m_bindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
{
	vkCreatePipelineLayout(Scope->GetDevice(), &LayoutInfo, VK_NULL_HANDLE, &m_pipelineLayout);

	pushConstants = std::vector(LayoutInfo.pPushConstantRanges, LayoutInfo.pPushConstantRanges + LayoutInfo.pushConstantRangeCount);
	CreateInfo.layout = m_pipelineLayout;
	m_subpass = CreateInfo.subpass;

	auto res = vkCreateGraphicsPipelines(Scope->GetDevice(), Cache, 1, &CreateInfo, VK_NULL_HANDLE, &m_pipeline);
	assert(res == VK_SUCCESS);
}

GVkPipeline::GVkPipeline(std::shared_ptr<RenderScope> InScope, VkPipelineLayoutCreateInfo LayoutInfo, VkComputePipelineCreateInfo CreateInfo, VkPipelineCache Cache)
	: Scope(InScope)
	, m_bindPoint(VK_PIPELINE_BIND_POINT_COMPUTE)
{
	vkCreatePipelineLayout(Scope->GetDevice(), &LayoutInfo, VK_NULL_HANDLE, &m_pipelineLayout);
	CreateInfo.layout = m_pipelineLayout;

	auto res = vkCreateComputePipelines(Scope->GetDevice(), Cache, 1, &CreateInfo, VK_NULL_HANDLE, &m_pipeline);
	assert(res == VK_SUCCESS);
}

GVkPipeline::~GVkPipeline()
{
	vkDestroyPipelineLayout(Scope->GetDevice(), m_pipelineLayout, VK_NULL_HANDLE);
	vkDestroyPipeline(Scope->GetDevice(), m_pipeline, VK_NULL_HANDLE);
}

std::string ComputePipelineDescriptor::HashString() const
{
	return CS.HashString();
}

std::shared_ptr<GVkPipeline> ComputePipelineDescriptor::Construct(std::shared_ptr<RenderScope> Scope)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = descriptorLayouts.size();
	pipelineLayoutCreateInfo.pSetLayouts = descriptorLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstants.size();
	pipelineLayoutCreateInfo.pPushConstantRanges = pushConstants.data();

	VkComputePipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	pipelineCreateInfo.stage.module = Scope->GetCache(CS);

	return std::make_shared<GVkPipeline>(Scope, pipelineLayoutCreateInfo, pipelineCreateInfo, Scope->GetCache(*this));
}

GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::SetVertexInputBindings(const std::vector<VkVertexInputBindingDescription>& bindings)
{
	_vertexInputBind = bindings;

	vertexInput.vertexBindingDescriptionCount = _vertexInputBind.size();
	vertexInput.pVertexBindingDescriptions = _vertexInputBind.data();

	return *this;
}

GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::SetVertexAttributeBindings(const std::vector<VkVertexInputAttributeDescription>& attributes)
{
	_vertexInputAttr = attributes;

	vertexInput.vertexAttributeDescriptionCount = _vertexInputAttr.size();
	vertexInput.pVertexAttributeDescriptions = _vertexInputAttr.data();

	return *this;
}

GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::SetPrimitiveTopology(VkPrimitiveTopology topology)
{
	inputAssembly.topology = topology;

	return *this;
}

GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::SetPolygonMode(VkPolygonMode mode)
{
	rasterizationState.polygonMode = mode;

	return *this;
}

GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::SetCullMode(VkCullModeFlags mode, VkFrontFace front)
{
	rasterizationState.cullMode = mode;
	rasterizationState.frontFace = front;

	return *this;
}

GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
	rasterizationState.depthBiasEnable = depthBiasConstantFactor != 0.f || depthBiasClamp != 0.f || depthBiasSlopeFactor != 0.f;
	rasterizationState.depthBiasConstantFactor = depthBiasConstantFactor;
	rasterizationState.depthBiasClamp = depthBiasClamp;
	rasterizationState.depthBiasSlopeFactor = depthBiasSlopeFactor;

	return *this;
}

GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::SetDepthState(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp)
{
	depthStencilState.depthTestEnable = depthTestEnable;
	depthStencilState.depthWriteEnable = depthWriteEnable;
	depthStencilState.depthCompareOp = depthCompareOp;

	return *this;
}

GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::SetSampling(VkSampleCountFlagBits samples)
{
	multisampleState.rasterizationSamples = samples;

	return *this;
}

GraphicsPipelineDescriptor& GraphicsPipelineDescriptor::SetRenderPass(std::shared_ptr<GVkRenderPass> RenderPass, uint32_t Subpass)
{
	subpass = Subpass;
	renderPass = RenderPass;
	blendAttachments = std::vector(RenderPass->GetColorAttachmentCount(Subpass), defaultAttachment);

	return *this;
}

VkPipelineColorBlendAttachmentState& GraphicsPipelineDescriptor::AttachmentBlendState(uint32_t Index)
{
	return blendAttachments[Index];
}

std::string GraphicsPipelineDescriptor::HashString() const
{
	std::string hashString;

	hashString += VS.HashString();
	hashString += GS.HashString();
	hashString += PS.HashString();

	hashString += std::string((char*)&subpass, sizeof(subpass));
	hashString += std::string((char*)&blendState, sizeof(blendState));
	hashString += std::string((char*)&vertexInput, sizeof(vertexInput));
	hashString += std::string((char*)&inputAssembly, sizeof(inputAssembly));
	hashString += std::string((char*)&viewportState, sizeof(viewportState));
	hashString += std::string((char*)&multisampleState, sizeof(multisampleState));
	hashString += std::string((char*)&depthStencilState, sizeof(depthStencilState));
	hashString += std::string((char*)&rasterizationState, sizeof(rasterizationState));
	hashString += std::string((char*)blendAttachments.data(), sizeof(VkPipelineColorBlendAttachmentState) * blendAttachments.size());

	return hashString;
}

std::shared_ptr<GVkPipeline> GraphicsPipelineDescriptor::Construct(std::shared_ptr<RenderScope> Scope)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = descriptorLayouts.size();
	pipelineLayoutCI.pSetLayouts = descriptorLayouts.data();
	pipelineLayoutCI.pushConstantRangeCount = pushConstants.size();
	pipelineLayoutCI.pPushConstantRanges = pushConstants.data();

	blendState.attachmentCount = blendAttachments.size();
	blendState.pAttachments = blendAttachments.data();

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	if (VS.IsValid())
	{
		VkPipelineShaderStageCreateInfo stageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		stageInfo.module = Scope->GetCache(VS);
		stageInfo.pName = "main";
		// stageInfo.pSpecializationInfo
		shaderStages.push_back(stageInfo);
	}

	if (GS.IsValid())
	{
		VkPipelineShaderStageCreateInfo stageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		stageInfo.module = Scope->GetCache(GS);
		stageInfo.pName = "main";
		// stageInfo.pSpecializationInfo
		shaderStages.push_back(stageInfo);
	}

	if (PS.IsValid())
	{
		VkPipelineShaderStageCreateInfo stageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stageInfo.module = Scope->GetCache(PS);
		stageInfo.pName = "main";
		// stageInfo.pSpecializationInfo
		shaderStages.push_back(stageInfo);
	}

	VkGraphicsPipelineCreateInfo pipelineCI{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineCI.subpass = subpass;
	pipelineCI.renderPass = renderPass->GetRenderPass();
	pipelineCI.pInputAssemblyState = &inputAssembly;
	pipelineCI.pVertexInputState = &vertexInput;
	pipelineCI.pRasterizationState = &rasterizationState;
	pipelineCI.pDepthStencilState = &depthStencilState;
	pipelineCI.pColorBlendState = &blendState;
	pipelineCI.pViewportState = &viewportState;
	pipelineCI.pDynamicState = &dynamicState;
	pipelineCI.pMultisampleState = &multisampleState;
	pipelineCI.stageCount = shaderStages.size();
	pipelineCI.pStages = shaderStages.data();

	return std::make_shared<GVkPipeline>(Scope, pipelineLayoutCI, pipelineCI, Scope->GetCache(*this));
}