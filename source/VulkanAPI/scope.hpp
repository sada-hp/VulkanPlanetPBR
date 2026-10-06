#pragma once
#include "resource_cache.hpp"
#include "queue.hpp"
#include "core.hpp"

#include <vulkan/vulkan.h>
#include "vma/vk_mem_alloc.h"

#include <map>

// dummy obj for interfaces
class IVkObj {};

class RenderScope
{
	friend class GVulkanBase;

private:
	const uint32_t FramesInFlight = 3u;

private:
	std::shared_ptr<GVkResourceCache> m_Cache = VK_NULL_HANDLE;
	std::map<VkQueueFlagBits, Queue> m_Queues = {};
	uint32_t m_ResourceIndex = 0;

	VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_LogicalDevice = VK_NULL_HANDLE;
	VmaAllocator m_Allocator = VK_NULL_HANDLE;
	VkInstance m_VkInstance = VK_NULL_HANDLE;

private:
	void _createMemoryAllocator();
	void _createVulkanInstance();

	bool _createLogicalDevice(VkPhysicalDevice device, const std::vector<const char*>& device_extensions);
	void _createDevice();
	void _collectQueues();

	VkBool32 _checkValidationLayerSupport() const;
	static VKAPI_ATTR VkBool32 VKAPI_CALL _debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

private:
	void IncrementFlightIndex();

public:
	RenderScope();
	~RenderScope();

	inline const VkPhysicalDevice& GetPhysicalDevice() const { return m_PhysicalDevice; }
	inline const VmaAllocator& GetAllocator() const { return m_Allocator; }
	inline const VkInstance& GetInstance() const { return m_VkInstance; }
	inline const VkDevice& GetDevice() const { return m_LogicalDevice; }

	inline const uint32_t& GetMaxFramesInFlight() const { return FramesInFlight; }
	inline const uint32_t& GetResourceIndex() const { return m_ResourceIndex; }

	inline const Queue& GetQueue(VkQueueFlagBits Type) const
	{
		if (m_Queues.contains(Type))
			return m_Queues.at(Type);
	
		return m_Queues.at(VK_QUEUE_GRAPHICS_BIT);
	}

public:
	template<typename T>
	decltype(auto) GetCache(const T& Object)
	{
		return m_Cache->Get(Object);
	}

public:
	static constexpr VkFormat GetColorFormat()
	{
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	}

	static constexpr VkFormat GetDepthFormat()
	{
		return VK_FORMAT_D32_SFLOAT;
	}
};