#pragma once
#include "semaphore.hpp"
#include "fence.hpp"
#include "scope.hpp"

struct GVkSwapchainState
{
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	VkAccessFlags2 access = VK_ACCESS_2_NONE;
};

class GVkSwapchain : public IVkObj
{
	struct _internalObj
	{
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		GVkSwapchainState state = {};
		std::shared_ptr<GVkSemaphore> waitSemaphore = VK_NULL_HANDLE;
	};

private:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	std::vector<_internalObj> m_swapchainResources = {};

	VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	VkExtent2D m_Extents;

	std::shared_ptr<GVkSemaphore> m_acquireSemaphore = VK_NULL_HANDLE;
	uint32_t m_SwapchainIndex = 0;

private:
	const _internalObj& _activeObj() const { return m_swapchainResources.at(m_SwapchainIndex); }
	_internalObj& _activeObj() { return m_swapchainResources.at(m_SwapchainIndex); }

private:
	void _createSurface(struct GLFWwindow* window);
	void _createSwapchain();
	void _clear();

public:
	GVkSwapchain(const std::shared_ptr<RenderScope>& Scope, struct GLFWwindow* window);
	~GVkSwapchain();

	std::shared_ptr<GVkSemaphore> GetAcquireSemaphore() const;
	const GVkSwapchainState& GetImageState() const;
	const VkImageView& GetImageView() const;
	const VkExtent2D& GetExtent() const;
	const VkImage& GetImage() const;

	void SetImageState(VkImageLayout Layout, VkPipelineStageFlags Stage, VkAccessFlags Access);

	void Present(std::shared_ptr<GVkSemaphore> waitSemaphore);
	bool QueryNextImage(std::shared_ptr<GVkFence> WaitFence);
};