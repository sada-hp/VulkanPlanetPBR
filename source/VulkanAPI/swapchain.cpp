#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include "swapchain.hpp"

GVkSwapchain::GVkSwapchain(const std::shared_ptr<RenderScope>& InScope, GLFWwindow* window)
	: Scope(InScope)
{
	_createSurface(window);
	_createSwapchain();
}

GVkSwapchain::~GVkSwapchain()
{
	_clear();
	vkDestroySurfaceKHR(Scope->GetInstance(), m_Surface, VK_NULL_HANDLE);
}

void GVkSwapchain::_clear()
{
	m_acquireFence = VK_NULL_HANDLE;

	for (auto& [image, view, state] : m_swapchainResources)
		vkDestroyImageView(Scope->GetDevice(), view, VK_NULL_HANDLE);

	if (m_Swapchain)
		vkDestroySwapchainKHR(Scope->GetDevice(), m_Swapchain, VK_NULL_HANDLE);

	m_Swapchain = VK_NULL_HANDLE;
	m_swapchainResources.clear();
}

void GVkSwapchain::_createSurface(GLFWwindow* window)
{
	auto res = glfwCreateWindowSurface(Scope->GetInstance(), window, VK_NULL_HANDLE, &m_Surface);
	assert(res == VK_SUCCESS);
}

void GVkSwapchain::_createFence()
{
	m_acquireFence = std::make_shared<GVkFence>(Scope, EFenceFlags::CreateSignaled);
}

void GVkSwapchain::_createSwapchain()
{
	_clear();

	VkSurfaceCapabilitiesKHR capabilities{};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Scope->GetPhysicalDevice(), m_Surface, &capabilities);

	if (capabilities.currentExtent.width > 0 && capabilities.currentExtent.height > 0)
	{
		uint32_t surfaceFormatCount = 0;
		std::vector<VkSurfaceFormatKHR> surfaceFormats = {};

		vkGetPhysicalDeviceSurfaceFormatsKHR(Scope->GetPhysicalDevice(), m_Surface, &surfaceFormatCount, VK_NULL_HANDLE);
		surfaceFormats.resize(surfaceFormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(Scope->GetPhysicalDevice(), m_Surface, &surfaceFormatCount, surfaceFormats.data());

		VkSurfaceFormatKHR surfaceFormat = surfaceFormats.front();

		for (auto& candidateFormat : surfaceFormats)
		{
			if (candidateFormat.format == VK_FORMAT_R8G8B8A8_UNORM && candidateFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			{
				surfaceFormat = candidateFormat;
				break;
			}
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.minImageCount = std::clamp(Scope->GetMaxFramesInFlight(), capabilities.minImageCount, capabilities.maxImageCount);
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.preTransform = capabilities.currentTransform;
		createInfo.imageArrayLayers = 1;
		createInfo.surface = m_Surface;
		createInfo.clipped = VK_TRUE;
#ifndef FORCE_VSYNC
		createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
#else
		createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
#endif
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageExtent = capabilities.currentExtent;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;

		auto res = vkCreateSwapchainKHR(Scope->GetDevice(), &createInfo, VK_NULL_HANDLE, &m_Swapchain);
		assert(res == VK_SUCCESS);
		m_Extents = capabilities.currentExtent;

		uint32_t imageCount = 0;
		vkGetSwapchainImagesKHR(Scope->GetDevice(), m_Swapchain, &imageCount, VK_NULL_HANDLE);

		m_swapchainResources.resize(imageCount);
		std::vector<VkImage> swapchainImages(imageCount);
		vkGetSwapchainImagesKHR(Scope->GetDevice(), m_Swapchain, &imageCount, swapchainImages.data());

		for (uint32_t i = 0; i < imageCount; i++)
		{
			m_swapchainResources[i].image = swapchainImages[i];

			VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.layerCount = 1;
			viewCreateInfo.format = createInfo.imageFormat;
			viewCreateInfo.image = swapchainImages[i];
			vkCreateImageView(Scope->GetDevice(), &viewCreateInfo, VK_NULL_HANDLE, &m_swapchainResources[i].view);
		}

		_createFence();
	}
}

void GVkSwapchain::_wait() const
{
	if (m_acquireFence)
		m_acquireFence->Wait();
}

const VkImage& GVkSwapchain::GetImage() const
{
	_wait();

	auto& object = _activeObj();
	return object.image;
}

const VkImageView& GVkSwapchain::GetImageView() const
{
	_wait();

	auto& object = _activeObj();
	return object.view;
}

const GVkSwapchainState& GVkSwapchain::GetImageState() const
{
	auto& object = _activeObj();
	return object.state;
}

const VkExtent2D& GVkSwapchain::GetExtent() const
{
	return m_Extents;
}

void GVkSwapchain::SetImageState(VkImageLayout Layout, VkPipelineStageFlags Stage, VkAccessFlags Access)
{
	auto& object = _activeObj();
	object.state.layout = Layout;
	object.state.access = Access;
	object.state.stage = Stage;
}

bool GVkSwapchain::QueryNextImage()
{
	if (!m_Swapchain)
		_createSwapchain();

	if (m_Swapchain)
	{
		m_acquireFence->Reset();
		VkResult SwapchainStatus = vkAcquireNextImageKHR(Scope->GetDevice(), m_Swapchain, UINT64_MAX, VK_NULL_HANDLE, m_acquireFence->GetFence(), &m_SwapchainIndex);

		if (SwapchainStatus != VK_SUCCESS)
		{
			m_acquireFence->Signal();
			_clear();
		}
	}

	return m_Swapchain != VK_NULL_HANDLE;
}

void GVkSwapchain::Present()
{
	_wait();

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.pImageIndices = &m_SwapchainIndex;
	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.swapchainCount = 1;
	vkQueuePresentKHR(Scope->GetQueue(VK_QUEUE_GRAPHICS_BIT).GetQueue(), &presentInfo);
}