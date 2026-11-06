#include "render_pass.hpp"
#include "image.hpp"

static VkPipelineStageFlags _pipelineStageFromLayout(VkImageLayout layout, bool Src)
{
	const std::unordered_map<VkImageLayout, VkPipelineStageFlags> stageTable = {
		{VK_IMAGE_LAYOUT_UNDEFINED, 0},
		{VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT },
		{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
		{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
		{VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT},
		{VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT},
		{VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, Src ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT},
		{VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, Src ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT},
		{VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT}
	};

	return stageTable.contains(layout) ? stageTable.at(layout) : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
}

static VkAccessFlags _pipelineAccessFromLayout(VkImageLayout layout, bool Src)
{
	const std::unordered_map<VkImageLayout, VkAccessFlags> accessTable = {
		{VK_IMAGE_LAYOUT_UNDEFINED, 0},
		{VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT },
		{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT},
		{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT},
		{VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT},
		{VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT},
		{VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_MEMORY_READ_BIT},
		{VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
		{VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT}
	};

	return accessTable.contains(layout) ? accessTable.at(layout) : VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
}

static bool _isDepthFormat(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT
		|| format == VK_FORMAT_D32_SFLOAT_S8_UINT
		|| format == VK_FORMAT_D24_UNORM_S8_UINT
		|| format == VK_FORMAT_D16_UNORM_S8_UINT
		|| format == VK_FORMAT_D16_UNORM;
}

GVkRenderPass::GVkRenderPass(std::shared_ptr<RenderScope> InScope, VkRenderPassCreateInfo CreateInfo, const std::vector<VkClearValue>& ClearValues)
	: Scope(InScope)
	, m_clearValues(ClearValues)
	, m_subpassCount(CreateInfo.subpassCount)
{
	for (uint32_t i = 0; i < CreateInfo.subpassCount; i++)
	{
		auto& subpass = CreateInfo.pSubpasses[i];

		m_colorAttachmentsCounts.push_back(subpass.colorAttachmentCount);
		for (uint32_t j = 0; j < subpass.colorAttachmentCount; j++)
		{
			auto& reference = subpass.pColorAttachments[j];
			auto& attachment = const_cast<VkAttachmentDescription&>(CreateInfo.pAttachments[reference.attachment]);

			if (!m_imageStates.contains(reference.attachment))
			{
				m_imageStates[reference.attachment].initial_layout = attachment.initialLayout;
				m_imageStates[reference.attachment].initial_stage  = _pipelineStageFromLayout(attachment.initialLayout, false);
				m_imageStates[reference.attachment].initial_access = _pipelineAccessFromLayout(attachment.initialLayout, false);

				m_imageStates[reference.attachment].next_layout = attachment.finalLayout;
				m_imageStates[reference.attachment].next_stage  = _pipelineStageFromLayout(attachment.finalLayout, false);
				m_imageStates[reference.attachment].next_access = _pipelineAccessFromLayout(attachment.finalLayout, false);
			}

			m_imageStates[reference.attachment].final_layout = reference.layout;
			m_imageStates[reference.attachment].final_stage = _pipelineStageFromLayout(reference.layout, true);
			m_imageStates[reference.attachment].final_access = _pipelineAccessFromLayout(reference.layout, true);

			// we handle explicit layout transitions in command buffer end render pass
			attachment.finalLayout = reference.layout;
		}

		for (uint32_t j = 0; j < subpass.inputAttachmentCount; j++)
		{
			auto& reference = subpass.pInputAttachments[j];
			auto& attachment = const_cast<VkAttachmentDescription&>(CreateInfo.pAttachments[reference.attachment]);

			if (!m_imageStates.contains(reference.attachment))
			{
				m_imageStates[reference.attachment].initial_layout = attachment.initialLayout;
				m_imageStates[reference.attachment].initial_stage  = _pipelineStageFromLayout(attachment.initialLayout, false);
				m_imageStates[reference.attachment].initial_access = _pipelineAccessFromLayout(attachment.initialLayout, false);

				m_imageStates[reference.attachment].next_layout = attachment.finalLayout;
				m_imageStates[reference.attachment].next_stage  = _pipelineStageFromLayout(attachment.finalLayout, false);
				m_imageStates[reference.attachment].next_access = _pipelineAccessFromLayout(attachment.finalLayout, false);
			}

			m_imageStates[reference.attachment].final_layout = reference.layout;
			m_imageStates[reference.attachment].final_stage = _pipelineStageFromLayout(reference.layout, true);
			m_imageStates[reference.attachment].final_access = _pipelineAccessFromLayout(reference.layout, true);

			// we handle explicit layout transitions in command buffer end render pass
			attachment.finalLayout = reference.layout;
		}

		if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED)
		{
			auto& reference = *subpass.pDepthStencilAttachment;
			auto& attachment = const_cast<VkAttachmentDescription&>(CreateInfo.pAttachments[reference.attachment]);

			if (!m_imageStates.contains(reference.attachment))
			{
				m_imageStates[reference.attachment].initial_layout = attachment.initialLayout;
				m_imageStates[reference.attachment].initial_stage  = _pipelineStageFromLayout(attachment.initialLayout, false);
				m_imageStates[reference.attachment].initial_access = _pipelineAccessFromLayout(attachment.initialLayout, false);

				m_imageStates[reference.attachment].next_layout = attachment.finalLayout;
				m_imageStates[reference.attachment].next_stage  = _pipelineStageFromLayout(attachment.finalLayout, false);
				m_imageStates[reference.attachment].next_access = _pipelineAccessFromLayout(attachment.finalLayout, false);
			}

			m_imageStates[reference.attachment].final_layout = reference.layout;
			m_imageStates[reference.attachment].final_stage = _pipelineStageFromLayout(reference.layout, true);
			m_imageStates[reference.attachment].final_access = _pipelineAccessFromLayout(reference.layout, true);

			// we handle explicit layout transitions in command buffer end render pass
			attachment.finalLayout = reference.layout;
		}
	}

	vkCreateRenderPass(Scope->GetDevice(), &CreateInfo, VK_NULL_HANDLE, &m_RenderPass);
}

GVkRenderPass::~GVkRenderPass()
{
	vkDestroyRenderPass(Scope->GetDevice(), m_RenderPass, VK_NULL_HANDLE);
}

const GVkAttachmentState& GVkRenderPass::_getAttachmentState(uint32_t Index) const
{
	return m_imageStates[Index];
}

uint32_t RenderPassDescriptor::AddAttachmentLoadOp(VkFormat Format, VkImageLayout InitialLayout, VkImageLayout FinalLayout, VkSampleCountFlagBits Samples)
{
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.initialLayout = InitialLayout;
	attachmentDescription.finalLayout = FinalLayout;
	attachmentDescription.samples = Samples;
	attachmentDescription.format = Format;
	m_attachmentDescriptions.push_back(attachmentDescription);
	m_clearValues.push_back(VkClearValue{});

	return m_attachmentDescriptions.size() - 1;
}

uint32_t RenderPassDescriptor::AddAttachmentDontCareOp(VkFormat Format, VkImageLayout FinalLayout, VkSampleCountFlagBits Samples)
{
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.finalLayout = FinalLayout;
	attachmentDescription.samples = Samples;
	attachmentDescription.format = Format;
	m_attachmentDescriptions.push_back(attachmentDescription);

	m_clearValues.push_back(VkClearValue{});

	return m_attachmentDescriptions.size() - 1;
}

uint32_t RenderPassDescriptor::AddAttachmentClearOp(VkFormat Format, VkClearValue ClearValue, VkImageLayout FinalLayout, VkSampleCountFlagBits Samples)
{
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.finalLayout = FinalLayout;
	attachmentDescription.samples = Samples;
	attachmentDescription.format = Format;

	m_attachmentDescriptions.push_back(attachmentDescription);
	m_clearValues.push_back(ClearValue);

	return m_attachmentDescriptions.size() - 1;
}

RenderPassDescriptor& RenderPassDescriptor::AddSubpass(const SubpassDescription& SubpassDescription)
{
	GSubpass Subpass;
	for (auto& colorIndex : SubpassDescription.colorAttachmentsIndices)
		Subpass.colorReferences.emplace_back(colorIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	for (auto& inputIndex : SubpassDescription.inputAttachmentsIndices)
		Subpass.inputReferences.emplace_back(inputIndex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	Subpass.depthReference = VkAttachmentReference{ SubpassDescription.depthAttachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	m_subpasses.push_back(std::move(Subpass));
	return *this;
}

std::vector<VkSubpassDependency> RenderPassDescriptor::buildSubpassDependencies()
{
	std::vector<VkSubpassDependency> dependencies(m_subpasses.size() + 1, VkSubpassDependency{});

	// build indices
	for (uint32_t i = 0; i < dependencies.size(); i++)
	{
		dependencies[i].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[i].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[i].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		if (i != dependencies.size() - 1)
			dependencies[i].dstSubpass = i;

		if (i != 0)
			dependencies[i].srcSubpass = i - 1;
	}

	// build dst stages
	for (uint32_t i = 0; i < m_subpasses.size(); i++)
	{
		const auto& subpass = m_subpasses[i];

		if (!subpass.colorReferences.empty())
		{
			dependencies[i].dstAccessMask |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
			dependencies[i].dstStageMask |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		}

		if (!subpass.inputReferences.empty())
		{
			dependencies[i].dstStageMask |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			dependencies[i].dstAccessMask |= VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT;
		}

		if (subpass.depthReference.attachment != VK_ATTACHMENT_UNUSED)
		{
			dependencies[i].dstAccessMask |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			dependencies[i].dstStageMask |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		}
	}

	// build subpass src stages
	for (uint32_t i = 0; i < m_subpasses.size(); i++)
	{
		const auto& subpass = m_subpasses[i];
		
		if (i == 0)
		{
			dependencies[i + 1].srcAccessMask = dependencies[0].dstAccessMask;
			dependencies[i + 1].srcStageMask = dependencies[0].dstStageMask;
		}
		else
		{
			for (auto& attachement : subpass.attachments())
			{
				// find previous use for attachment
				for (uint32_t j = i - 1; j < m_subpasses.size(); j++)
				{
					const auto& previous = m_subpasses[j];

					VkPipelineStageFlags2 srcStage; VkAccessFlags2 srcAccess;
					if (previous.contains(attachement, srcStage, srcAccess))
					{
						dependencies[i + 1].srcAccessMask |= srcAccess;
						dependencies[i + 1].srcStageMask |= srcStage;
						break;
					}
				}
			}
		}
	}

	// external sync
	dependencies.front().srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
	dependencies.back().dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

	for (uint32_t i = 0; i < dependencies.size() - 1; i++)
	{
		dependencies.back().srcAccessMask |= dependencies[i].dstAccessMask;
		dependencies.back().srcStageMask |= dependencies[i].dstStageMask;
	}

	return dependencies;
}

std::vector<VkSubpassDescription> RenderPassDescriptor::buildSubpassDescriptions()
{
	std::vector<VkSubpassDescription> subpassDescriptions;
	subpassDescriptions.reserve(m_subpasses.size());

	for (auto& Subpass : m_subpasses)
	{
		VkSubpassDescription subpassDescription{};
		subpassDescription.colorAttachmentCount = Subpass.colorReferences.size();
		subpassDescription.pColorAttachments = Subpass.colorReferences.data();
		subpassDescription.inputAttachmentCount = Subpass.inputReferences.size();
		subpassDescription.pInputAttachments = Subpass.inputReferences.data();
		subpassDescription.pDepthStencilAttachment = &Subpass.depthReference;
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions.push_back(subpassDescription);
	}

	return subpassDescriptions;
}

std::shared_ptr<GVkRenderPass> RenderPassDescriptor::Construct(std::shared_ptr<RenderScope> Scope)
{
	if (m_subpasses.empty())
	{
		SubpassDescription SubpassDS;

		uint32_t index = 0;
		for (auto& attachment : m_attachmentDescriptions)
		{
			if (_isDepthFormat(attachment.format))
				SubpassDS.depthAttachmentIndex = index;
			else
				SubpassDS.colorAttachmentsIndices.insert(index);

			index++;
		}

		AddSubpass(SubpassDS);
	}

	std::vector<VkSubpassDescription> subpassDescriptions = buildSubpassDescriptions();
	std::vector<VkSubpassDependency> subpassDependencies  = buildSubpassDependencies();

	VkRenderPassCreateInfo renderPassCreateInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassCreateInfo.attachmentCount = m_attachmentDescriptions.size();
	renderPassCreateInfo.pAttachments = m_attachmentDescriptions.data();
	renderPassCreateInfo.dependencyCount = subpassDependencies.size();
	renderPassCreateInfo.pDependencies = subpassDependencies.data();
	renderPassCreateInfo.subpassCount = subpassDescriptions.size();
	renderPassCreateInfo.pSubpasses = subpassDescriptions.data();

	return std::shared_ptr<GVkRenderPass>(new GVkRenderPass(Scope, renderPassCreateInfo, m_clearValues));
}