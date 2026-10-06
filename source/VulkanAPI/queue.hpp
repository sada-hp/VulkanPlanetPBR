#pragma once
#include <vulkan/vulkan.h>

class Queue
{
private:
	uint32_t m_familyIndex = 0;
	VkQueue m_vkQueue = VK_NULL_HANDLE;
	VkDevice m_vkDevice = VK_NULL_HANDLE;
	VkCommandPool m_CmdPool = VK_NULL_HANDLE;

public:
	Queue(const VkDevice& device, uint32_t family);
	~Queue();

	const uint32_t& GetFamilyIndex() const { return m_familyIndex; };
	const VkQueue& GetQueue() const { return m_vkQueue; };

	void allocateCommandBuffer(VkCommandBuffer* cmd) const;
	void freeCommandBuffer(VkCommandBuffer cmd) const;

	void submitCommandBuffer(VkCommandBuffer cmd, VkFence fence) const;
};