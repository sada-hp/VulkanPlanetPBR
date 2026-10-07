#pragma once
#include "render_pass.hpp"
#include "shader.hpp"
#include "scope.hpp"

class GVkRenderPass;

class GVkPipeline : public IVkObj
{
	friend class GVkCommandBuffer;

private:
	uint32_t m_subpass = 0;
	VkPipelineBindPoint m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	VkPipeline m_pipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	std::vector<VkPushConstantRange> pushConstants;

private:
	const std::vector<VkPushConstantRange>& _getPushConstants() const {
		return pushConstants;
	};

public:
	GVkPipeline(std::shared_ptr<RenderScope> Scope, VkPipelineLayoutCreateInfo LayoutInfo, VkGraphicsPipelineCreateInfo CreateInfo, VkPipelineCache Cache);
	GVkPipeline(std::shared_ptr<RenderScope> Scope, VkPipelineLayoutCreateInfo LayoutInfo, VkComputePipelineCreateInfo CreateInfo, VkPipelineCache Cache = VK_NULL_HANDLE);
	virtual ~GVkPipeline();

	const VkPipelineLayout& GetLayout() const { return m_pipelineLayout; };
	const VkPipeline& GetPipeline() const { return m_pipeline; };

	VkPipelineBindPoint GetBindPoint() const { return m_bindPoint; }
	uint32_t GetSubpass() const { return m_subpass; }
};

class IPipelineDescriptor
{
protected:
	std::vector<VkDescriptorSetLayout> descriptorLayouts{};
	std::vector<VkPushConstantRange> pushConstants;

public:
	virtual ~IPipelineDescriptor() {};
	virtual std::string HashString() const = 0;
	virtual std::shared_ptr<GVkPipeline> Construct(std::shared_ptr<RenderScope>) = 0;

public:
	template<typename T> 
	void AddPushConstants(VkShaderStageFlags Stages)
	{
		VkPushConstantRange range{};
		range.stageFlags = Stages;
		range.size = sizeof(T);

		range.offset = 0;
		for (auto& push : pushConstants)
			range.offset = std::max(range.offset, push.offset + push.size);

		pushConstants.push_back(range);
	}

	void AddDescriptorLayout(VkDescriptorSetLayout layout)
	{
		descriptorLayouts.push_back(layout);
	}
};

class ComputePipelineDescriptor : public IPipelineDescriptor
{
public:
	GVkShader<VK_SHADER_STAGE_COMPUTE_BIT> CS;

public:
	std::string HashString() const override;
	std::shared_ptr<GVkPipeline> Construct(std::shared_ptr<RenderScope>) override;
};

class GraphicsPipelineDescriptor : public IPipelineDescriptor
{
public:
	GraphicsPipelineDescriptor& SetVertexInputBindings(const std::vector<VkVertexInputBindingDescription>& bindings);

	GraphicsPipelineDescriptor& SetVertexAttributeBindings(const std::vector<VkVertexInputAttributeDescription>& attributes);

	GraphicsPipelineDescriptor& SetPrimitiveTopology(VkPrimitiveTopology topology);

	GraphicsPipelineDescriptor& SetPolygonMode(VkPolygonMode mode);

	GraphicsPipelineDescriptor& SetCullMode(VkCullModeFlags mode, VkFrontFace front = VK_FRONT_FACE_COUNTER_CLOCKWISE);

	GraphicsPipelineDescriptor& SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);

	GraphicsPipelineDescriptor& SetDepthState(VkBool32 depthTestEnable, VkBool32 depthWriteEnable = VK_TRUE, VkCompareOp depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL);

	GraphicsPipelineDescriptor& SetSampling(VkSampleCountFlagBits samples);

	GraphicsPipelineDescriptor& SetRenderPass(std::shared_ptr<GVkRenderPass> RenderPass, uint32_t Subpass = 0);

	VkPipelineColorBlendAttachmentState& AttachmentBlendState(uint32_t Index);

	std::string HashString() const override;
	std::shared_ptr<GVkPipeline> Construct(std::shared_ptr<RenderScope>) override;

public:
	GVkShader<VK_SHADER_STAGE_VERTEX_BIT> VS;
	GVkShader<VK_SHADER_STAGE_GEOMETRY_BIT> GS;
	GVkShader<VK_SHADER_STAGE_FRAGMENT_BIT> PS;

private:
	uint32_t subpass = 0;
	std::shared_ptr<GVkRenderPass> renderPass = VK_NULL_HANDLE;
	std::vector<VkVertexInputBindingDescription> _vertexInputBind = {};
	std::vector<VkVertexInputAttributeDescription> _vertexInputAttr = {};

	VkPipelineVertexInputStateCreateInfo vertexInput{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		VK_NULL_HANDLE,
		0u,
		0u,
		VK_NULL_HANDLE,
		0u,
		VK_NULL_HANDLE
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		VK_NULL_HANDLE,
		0u,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FALSE
	};

	VkPipelineRasterizationStateCreateInfo rasterizationState{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		VK_NULL_HANDLE,
		0u,
		VK_FALSE,
		VK_FALSE,
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_NONE,
		VK_FRONT_FACE_COUNTER_CLOCKWISE,
		VK_FALSE,
		0.f,
		0.f,
		0.f,
		1.f
	};

	const VkPipelineColorBlendAttachmentState defaultAttachment{
		VK_FALSE,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_ZERO,
		VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_ZERO,
		VK_BLEND_OP_ADD,
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	std::vector<VkPipelineColorBlendAttachmentState> blendAttachments{ 
		defaultAttachment
	};

	VkPipelineColorBlendStateCreateInfo blendState{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		VK_NULL_HANDLE,
		0u,
		VK_FALSE,
		VK_LOGIC_OP_COPY,
		static_cast<uint32_t>(blendAttachments.size()),
		blendAttachments.data(),
		0.f
	};

	const VkStencilOpState defaultStencil{
		VK_STENCIL_OP_KEEP,
		VK_STENCIL_OP_KEEP,
		VK_STENCIL_OP_KEEP,
		VK_COMPARE_OP_LESS_OR_EQUAL,
		0xff,
		0x0,
		0x0
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilState{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		VK_NULL_HANDLE,
		0u,
		VK_TRUE,
		VK_TRUE,
		VK_COMPARE_OP_GREATER_OR_EQUAL,
		VK_FALSE,
		VK_FALSE,
		defaultStencil,
		defaultStencil,
		0.f,
		1.f
	};

	VkPipelineViewportStateCreateInfo viewportState{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		VK_NULL_HANDLE,
		0u,
		1u,
		VK_NULL_HANDLE,
		1u,
		VK_NULL_HANDLE
	};

	const std::array<VkDynamicState, 2> dynamics{ 
		VK_DYNAMIC_STATE_VIEWPORT, 
		VK_DYNAMIC_STATE_SCISSOR 
	};

	VkPipelineDynamicStateCreateInfo dynamicState{
		VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		VK_NULL_HANDLE,
		0,
		static_cast<uint32_t>(dynamics.size()),
		dynamics.data()
	};

	VkPipelineMultisampleStateCreateInfo multisampleState{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		VK_NULL_HANDLE,
		0u,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FALSE,
		0.f,
		VK_NULL_HANDLE,
		VK_FALSE,
		VK_FALSE
	};
};