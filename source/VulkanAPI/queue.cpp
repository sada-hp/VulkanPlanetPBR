#include "queue.hpp"

Queue::Queue(const VkDevice& inDevice, uint32_t inFamily)
	: m_vkDevice(inDevice), m_familyIndex(inFamily)
{
	vkGetDeviceQueue(m_vkDevice, m_familyIndex, 0, &m_vkQueue);

	VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = m_familyIndex;

	vkCreateCommandPool(m_vkDevice, &poolInfo, VK_NULL_HANDLE, &m_CmdPool);
}

Queue::~Queue()
{
	vkDestroyCommandPool(m_vkDevice, m_CmdPool, VK_NULL_HANDLE);
}

void Queue::allocateCommandBuffer(VkCommandBuffer* cmd) const
{
	VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_CmdPool;
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(m_vkDevice, &allocInfo, cmd);
}

void Queue::freeCommandBuffer(VkCommandBuffer cmd) const
{
	vkFreeCommandBuffers(m_vkDevice, m_CmdPool, 1, &cmd);
}

void Queue::submitCommandBuffer(VkCommandBuffer cmd, VkFence fence) const
{
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	vkQueueSubmit(m_vkQueue, 1, &submitInfo, fence);
}