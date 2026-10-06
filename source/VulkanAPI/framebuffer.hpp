#pragma once
#include "scope.hpp"
#include "render_pass.hpp"
#include "image.hpp"

class GVkFramebuffer : public IVkObj
{
	struct _internalObj
	{
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
	};

private:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;

	std::vector<std::shared_ptr<GVkImageView>> m_views = {};
	std::vector<_internalObj> m_flightResources = {};
	VkExtent3D m_fbExtents = {};
	VkRect2D m_fbRect = {};

private:
	size_t _activeIndex() const { return Scope->GetResourceIndex() % m_flightResources.size(); }
	const _internalObj& _activeObj() const { return m_flightResources.at(_activeIndex()); }
	_internalObj& _activeObj() { return m_flightResources.at(_activeIndex()); }

public:
	GVkFramebuffer(std::shared_ptr<RenderScope> Scope, std::shared_ptr<GVkRenderPass> RenderPass, const std::vector<std::shared_ptr<GVkImageView>>& Views);
	GVkFramebuffer(std::shared_ptr<RenderScope> Scope, std::shared_ptr<GVkRenderPass> RenderPass, const std::vector<std::shared_ptr<GVkImage>>& Images);
	virtual ~GVkFramebuffer();

	const VkExtent3D& GetExtents() const { return m_fbExtents; }
	const VkRect2D& GetRenderArea() const { return m_fbRect; }

	const VkFramebuffer& GetFramebuffer();
	const std::vector<std::shared_ptr<GVkImageView>>& GetViews() const;
};