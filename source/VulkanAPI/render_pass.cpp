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
				m_imageStates[reference.attachment].initial_layout = reference.layout;
				m_imageStates[reference.attachment].initial_stage = _pipelineStageFromLayout(reference.layout, false);
				m_imageStates[reference.attachment].initial_access = _pipelineAccessFromLayout(reference.layout, false);

				m_imageStates[reference.attachment].next_layout = attachment.finalLayout;
				m_imageStates[reference.attachment].next_stage = _pipelineStageFromLayout(attachment.finalLayout, false);
				m_imageStates[reference.attachment].next_access = _pipelineAccessFromLayout(attachment.finalLayout, false);

				// we handle explicit layout transitions in command buffer begin render pass
				attachment.initialLayout = reference.layout;
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
				m_imageStates[reference.attachment].initial_layout = reference.layout;
				m_imageStates[reference.attachment].initial_stage = _pipelineStageFromLayout(reference.layout, false);
				m_imageStates[reference.attachment].initial_access = _pipelineAccessFromLayout(reference.layout, false);

				m_imageStates[reference.attachment].next_layout = attachment.finalLayout;
				m_imageStates[reference.attachment].next_stage = _pipelineStageFromLayout(attachment.finalLayout, false);
				m_imageStates[reference.attachment].next_access = _pipelineAccessFromLayout(attachment.finalLayout, false);

				// we handle explicit layout transitions in command buffer begin render pass
				attachment.initialLayout = reference.layout;
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
				m_imageStates[reference.attachment].initial_layout = reference.layout;
				m_imageStates[reference.attachment].initial_stage = _pipelineStageFromLayout(reference.layout, false);
				m_imageStates[reference.attachment].initial_access = _pipelineAccessFromLayout(reference.layout, false);

				m_imageStates[reference.attachment].next_layout = attachment.finalLayout;
				m_imageStates[reference.attachment].next_stage = _pipelineStageFromLayout(attachment.finalLayout, false);
				m_imageStates[reference.attachment].next_access = _pipelineAccessFromLayout(attachment.finalLayout, false);

				// we handle explicit layout transitions in command buffer begin render pass
				attachment.initialLayout = reference.layout;
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

RenderPassDescriptor& RenderPassDescriptor::AddSubpass(const GVkSubpassDescription& SubpassDescription)
{
	GVkSubpass Subpass;
	for (auto& colorIndex : SubpassDescription.colorAttachmentsIndices)
		Subpass.colorReferences.emplace_back(colorIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	for (auto& inputIndex : SubpassDescription.inputAttachmentsIndices)
		Subpass.inputReferences.emplace_back(inputIndex, VK_IMAGE_LAYOUT_GENERAL);

	Subpass.depthReference = VkAttachmentReference{ SubpassDescription.depthAttachmentIndex, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL };
	Subpass.Index = m_subpasses.size();

	m_subpasses.push_back(Subpass);
	return *this;
}

std::pair<VkPipelineStageFlags, VkAccessFlags> RenderPassDescriptor::find_ref(const GVkSubpass& subpass, uint32_t index)
{
	auto colorRef = std::find_if(subpass.colorReferences.begin(), subpass.colorReferences.end(), [index](const VkAttachmentReference& ref)
	{
		return ref.attachment == index;
	});

	if (colorRef != subpass.colorReferences.end())
		return std::pair{ _pipelineStageFromLayout(colorRef->layout, true), _pipelineAccessFromLayout(colorRef->layout, true) };

	auto inputRef = std::find_if(subpass.inputReferences.begin(), subpass.inputReferences.end(), [index](const VkAttachmentReference& ref)
	{
		return ref.attachment == index;
	});

	if (inputRef != subpass.inputReferences.end())
		return std::pair{ _pipelineStageFromLayout(inputRef->layout, true), _pipelineAccessFromLayout(inputRef->layout, true) };

	if (index == subpass.depthReference.attachment)
		return std::pair{ _pipelineStageFromLayout(subpass.depthReference.layout, true), _pipelineAccessFromLayout(subpass.depthReference.layout, true) };

	return std::pair{ 0, 0 };
}

std::shared_ptr<GVkRenderPass> RenderPassDescriptor::Construct(std::shared_ptr<RenderScope> Scope)
{
	if (m_subpasses.empty())
	{
		GVkSubpassDescription SubpassDS;

		uint32_t index = 0;
		for (auto& attachment : m_attachmentDescriptions)
		{
			if (attachment.format == VK_FORMAT_D24_UNORM_S8_UINT
			|| attachment.format == VK_FORMAT_D32_SFLOAT_S8_UINT
			|| attachment.format == VK_FORMAT_D16_UNORM_S8_UINT
			|| attachment.format == VK_FORMAT_D32_SFLOAT
			|| attachment.format == VK_FORMAT_D16_UNORM)
			{
				SubpassDS.depthAttachmentIndex = index;
			}
			else
			{
				SubpassDS.colorAttachmentsIndices.push_back(index);
			}

			index++;
		}

		AddSubpass(SubpassDS);
	}

	std::vector<VkSubpassDescription> subpassDescriptions;
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

	std::vector<VkSubpassDependency> subpassDependencies;

	GVkSubpass& firstSubpass = m_subpasses.front();
	VkSubpassDependency firstExternalDependency{};
	firstExternalDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	firstExternalDependency.dstSubpass = firstSubpass.Index;
	firstExternalDependency.dependencyFlags = 0;

	for (auto& ref : firstSubpass.colorReferences)
	{
		auto& attachment = m_attachmentDescriptions[ref.attachment];
		firstExternalDependency.dstStageMask |= _pipelineStageFromLayout(ref.layout, false);
		firstExternalDependency.dstAccessMask |= _pipelineAccessFromLayout(ref.layout, false);
	}

	for (auto& ref : firstSubpass.inputReferences)
	{
		auto& attachment = m_attachmentDescriptions[ref.attachment];
		firstExternalDependency.dstStageMask |= _pipelineStageFromLayout(ref.layout, false);
		firstExternalDependency.dstAccessMask |= _pipelineAccessFromLayout(ref.layout, false);
	}

	if (firstSubpass.depthReference.attachment != VK_ATTACHMENT_UNUSED)
	{
		auto& attachment = m_attachmentDescriptions[firstSubpass.depthReference.attachment];
		firstExternalDependency.dstStageMask |= _pipelineStageFromLayout(firstSubpass.depthReference.layout, false);
		firstExternalDependency.dstAccessMask |= _pipelineAccessFromLayout(firstSubpass.depthReference.layout, false);
	}

	subpassDependencies.push_back(firstExternalDependency);

	for (uint32_t i = 1; i < m_subpasses.size(); i++)
	{
		auto& PrevSubpass = m_subpasses[i - 1];
		auto& Subpass = m_subpasses[i];

		VkSubpassDependency subpassDependency{};
		subpassDependency.srcSubpass = PrevSubpass.Index;
		subpassDependency.dstSubpass = Subpass.Index;
		subpassDependency.dependencyFlags = 0;

		for (auto& ref : firstSubpass.colorReferences)
		{
			auto pair = find_ref(PrevSubpass, ref.attachment);
			subpassDependency.srcStageMask |= pair.first;
			subpassDependency.srcAccessMask |= pair.second;

			subpassDependency.dstStageMask |= _pipelineStageFromLayout(ref.layout, false);
			subpassDependency.dstAccessMask |= _pipelineAccessFromLayout(ref.layout, false);
		}

		for (auto& ref : firstSubpass.inputReferences)
		{
			auto pair = find_ref(PrevSubpass, ref.attachment);
			subpassDependency.srcStageMask |= pair.first;
			subpassDependency.srcAccessMask |= pair.second;

			subpassDependency.dstStageMask |= _pipelineStageFromLayout(ref.layout, false);
			subpassDependency.dstAccessMask |= _pipelineAccessFromLayout(ref.layout, false);
		}

		if (firstSubpass.depthReference.attachment != VK_ATTACHMENT_UNUSED)
		{
			auto pair = find_ref(PrevSubpass, firstSubpass.depthReference.attachment);
			subpassDependency.srcStageMask |= pair.first;
			subpassDependency.srcAccessMask |= pair.second;

			subpassDependency.dstStageMask |= _pipelineStageFromLayout(firstSubpass.depthReference.layout, false);
			subpassDependency.dstAccessMask |= _pipelineAccessFromLayout(firstSubpass.depthReference.layout, false);
		}

		subpassDependencies.push_back(subpassDependency);
	}

	GVkSubpass& lastSubpass = m_subpasses.back();
	VkSubpassDependency lastExternalDependency{};
	lastExternalDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	lastExternalDependency.srcSubpass = lastSubpass.Index;
	lastExternalDependency.dependencyFlags = 0;

	for (auto& ref : lastSubpass.colorReferences)
	{
		auto& attachment = m_attachmentDescriptions[ref.attachment];
		lastExternalDependency.srcStageMask |= _pipelineStageFromLayout(ref.layout, true);
		lastExternalDependency.srcAccessMask |= _pipelineAccessFromLayout(ref.layout, true);
	}

	for (auto& ref : lastSubpass.inputReferences)
	{
		auto& attachment = m_attachmentDescriptions[ref.attachment];
		lastExternalDependency.srcStageMask |= _pipelineStageFromLayout(ref.layout, true);
		lastExternalDependency.srcAccessMask |= _pipelineAccessFromLayout(ref.layout, true);
	}

	if (lastSubpass.depthReference.attachment != VK_ATTACHMENT_UNUSED)
	{
		auto& attachment = m_attachmentDescriptions[lastSubpass.depthReference.attachment];
		lastExternalDependency.srcStageMask |= _pipelineStageFromLayout(lastSubpass.depthReference.layout, true);
		lastExternalDependency.srcAccessMask |= _pipelineAccessFromLayout(lastSubpass.depthReference.layout, true);
	}

	subpassDependencies.push_back(lastExternalDependency);

	VkRenderPassCreateInfo renderPassCreateInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassCreateInfo.attachmentCount = m_attachmentDescriptions.size();
	renderPassCreateInfo.pAttachments = m_attachmentDescriptions.data();
	renderPassCreateInfo.dependencyCount = subpassDependencies.size();
	renderPassCreateInfo.pDependencies = subpassDependencies.data();
	renderPassCreateInfo.subpassCount = subpassDescriptions.size();
	renderPassCreateInfo.pSubpasses = subpassDescriptions.data();

	return std::shared_ptr<GVkRenderPass>(new GVkRenderPass(Scope, renderPassCreateInfo, m_clearValues));
}