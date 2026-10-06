#include "framebuffer.hpp"

GVkFramebuffer::GVkFramebuffer(std::shared_ptr<RenderScope> InScope, std::shared_ptr<GVkRenderPass> RenderPass, const std::vector<std::shared_ptr<GVkImageView>>& Views)
	: Scope(InScope), m_views(Views)
{
	std::vector<VkImageView> ImageViews;
	
	bool bInFlight = false;

	for (auto& view : m_views)
	{
		bInFlight |= view->GetRoot()->IsInFlight();
		m_fbExtents.width = std::max(m_fbExtents.width, view->GetRoot()->GetExtent().width);
		m_fbExtents.height = std::max(m_fbExtents.height, view->GetRoot()->GetExtent().height);
		m_fbExtents.depth = std::max(m_fbExtents.depth, view->GetRoot()->GetExtent().depth);
	}

	VkFramebufferCreateInfo framebufferCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferCreateInfo.renderPass = RenderPass->GetRenderPass();
	framebufferCreateInfo.width = m_fbExtents.width;
	framebufferCreateInfo.height = m_fbExtents.height;
	framebufferCreateInfo.layers = m_fbExtents.depth;

	m_fbRect.extent.width = m_fbExtents.width;
	m_fbRect.extent.height = m_fbExtents.height;

	m_flightResources.resize(bInFlight ? Scope->GetMaxFramesInFlight() : 1);

	uint32_t index = 0;
	for (auto& object : m_flightResources)
	{
		std::vector<VkImageView> ImageViews;
		for (auto& view : m_views)
			ImageViews.push_back(view->At(index));

		framebufferCreateInfo.attachmentCount = ImageViews.size();
		framebufferCreateInfo.pAttachments = ImageViews.data();

		vkCreateFramebuffer(Scope->GetDevice(), &framebufferCreateInfo, VK_NULL_HANDLE, &object.framebuffer);
		index++;
	}
}

GVkFramebuffer::GVkFramebuffer(std::shared_ptr<RenderScope> InScope, std::shared_ptr<GVkRenderPass> RenderPass, const std::vector<std::shared_ptr<GVkImage>>& Images)
	: GVkFramebuffer(InScope, RenderPass, [Images]() -> std::vector<std::shared_ptr<GVkImageView>>
		{
			std::vector<std::shared_ptr<GVkImageView>> views;
			for (auto& image : Images) views.push_back(GVkImage::ToView(image));
			return views;
		}()
	)
{
}

GVkFramebuffer::~GVkFramebuffer()
{
	for (auto& object : m_flightResources)
	{
		vkDestroyFramebuffer(Scope->GetDevice(), object.framebuffer, VK_NULL_HANDLE);
	}
}

const VkFramebuffer& GVkFramebuffer::GetFramebuffer()
{
	auto& object = _activeObj();
	return object.framebuffer;
}

const std::vector<std::shared_ptr<GVkImageView>>& GVkFramebuffer::GetViews() const
{
	return m_views;
}