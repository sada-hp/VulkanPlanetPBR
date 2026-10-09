#pragma once
#include "scope.hpp"

struct GVkAttachmentState
{
	VkPipelineStageFlags2 final_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkImageLayout final_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkAccessFlags2 final_access = VK_ACCESS_NONE;

	VkPipelineStageFlags2 initial_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkAccessFlags2 initial_access = VK_ACCESS_NONE;

	VkPipelineStageFlags2 next_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkImageLayout next_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkAccessFlags2 next_access = VK_ACCESS_NONE;
};

class GVkRenderPass : public IVkObj
{
	friend class GVkCommandBuffer;
	friend class RenderPassDescriptor;

private:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;

	std::vector<VkClearValue> m_clearValues = {};
	uint32_t m_subpassCount = 0;

	VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	mutable std::map<uint32_t, GVkAttachmentState> m_imageStates;
	std::vector<uint32_t> m_colorAttachmentsCounts;

private:
	GVkRenderPass(std::shared_ptr<RenderScope> Scope, VkRenderPassCreateInfo CreateInfo, const std::vector<VkClearValue>& ClearValues = {});
	const GVkAttachmentState& _getAttachmentState(uint32_t Index) const;

public:
	virtual ~GVkRenderPass();

	const std::vector<VkClearValue>& GetClearValues() const { return m_clearValues; }
	const VkRenderPass& GetRenderPass() const { return m_RenderPass; }
	uint32_t GetColorAttachmentCount(uint32_t Subpass) const { return m_colorAttachmentsCounts[Subpass]; }
	uint32_t GetSubpassCount() const { return m_subpassCount; }
};

struct GVkSubpassDescription
{
	std::vector<uint32_t> colorAttachmentsIndices = {};
	std::vector<uint32_t> inputAttachmentsIndices = {};
	uint32_t depthAttachmentIndex = VK_ATTACHMENT_UNUSED;
};

class RenderPassDescriptor
{
	struct GVkSubpass
	{
		std::vector<VkAttachmentReference> colorReferences = {};
		std::vector<VkAttachmentReference> inputReferences = {};
		VkAttachmentReference depthReference = { VK_ATTACHMENT_UNUSED };
		uint32_t Index = VK_SUBPASS_EXTERNAL;
	};

	std::vector<VkAttachmentDescription> m_attachmentDescriptions = {};
	std::vector<VkClearValue> m_clearValues = {};
	std::vector<GVkSubpass> m_subpasses = {};

private:
	std::pair<VkPipelineStageFlags, VkAccessFlags> find_ref(const GVkSubpass& subpass, uint32_t index);

public:
	uint32_t AddAttachmentLoadOp(VkFormat Format, VkImageLayout InitialLayout, VkImageLayout FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT);
	uint32_t AddAttachmentDontCareOp(VkFormat Format, VkImageLayout FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT);
	uint32_t AddAttachmentClearOp(VkFormat Format, VkClearValue ClearValue = {}, VkImageLayout FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT);
	
	RenderPassDescriptor& AddSubpass(const GVkSubpassDescription& Subpass);

	std::shared_ptr<GVkRenderPass> Construct(std::shared_ptr<RenderScope> Scope);
};