#include "queue.hpp"

Queue::Queue(const VkDevice& inDevice, VkQueueFlags flags, uint32_t inFamily)
	: m_vkDevice(inDevice), m_familyIndex(inFamily)
{
	vkGetDeviceQueue(m_vkDevice, m_familyIndex, 0, &m_vkQueue);

	VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = m_familyIndex;

	vkCreateCommandPool(m_vkDevice, &poolInfo, VK_NULL_HANDLE, &m_CmdPool);

	if (flags & VK_QUEUE_COMPUTE_BIT)
		m_supportedFlags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;

	if (flags & VK_QUEUE_TRANSFER_BIT)
		m_supportedFlags |= VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT;

	if (flags & VK_QUEUE_GRAPHICS_BIT)
		m_supportedFlags = ~0ull;
}

Queue::~Queue()
{
	vkDestroyCommandPool(m_vkDevice, m_CmdPool, VK_NULL_HANDLE);
}

bool Queue::IsStageSupported(VkPipelineStageFlags2 Stage) const
{
	return (m_supportedFlags & Stage) != 0;
}

void Queue::allocateCommandBuffer(VkCommandBuffer* cmd) const
{
	VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_CmdPool;
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(m_vkDevice, &allocInfo, cmd);
}

void Queue::allocateCommandBuffer2(VkCommandBuffer* cmd) const
{
	VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	allocInfo.commandPool = m_CmdPool;
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(m_vkDevice, &allocInfo, cmd);
}

void Queue::freeCommandBuffer(VkCommandBuffer cmd) const
{
	vkFreeCommandBuffers(m_vkDevice, m_CmdPool, 1, &cmd);
}

void Queue::submitCommandBuffer(VkCommandBuffer cmd, VkFence fence, const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkPipelineStageFlags>& waitStages, const std::vector<VkSemaphore>& signalSemaphores) const
{
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	submitInfo.waitSemaphoreCount = waitSemaphores.size();
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();

	submitInfo.signalSemaphoreCount = signalSemaphores.size();
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	vkQueueSubmit(m_vkQueue, 1, &submitInfo, fence);
}

void Queue::submit(const std::vector<VkSubmitInfo>& submissions, VkFence fence) const
{
	if (!submissions.empty())
		vkQueueSubmit(m_vkQueue, submissions.size(), submissions.data(), fence);
}