#pragma once
#include "VulkanAPI/command_buffer.hpp"
#include "VulkanAPI/buffer.hpp"
#include "VulkanAPI/scope.hpp"

class GGPuMemoryPool
{
	std::vector<std::shared_ptr<GVkBufferView>> _views;
	std::shared_ptr<GVkCommandBuffer> _commandBuffer;

protected:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	std::shared_ptr<GVkBuffer> m_Buffer = VK_NULL_HANDLE;
	EBufferFlags m_Flags;
	size_t m_Stride = 0;

private:
	size_t _findSuitableIndex();

public:
	GGPuMemoryPool(std::shared_ptr<RenderScope> Scope, EBufferFlags flags, size_t stride, size_t count);
	~GGPuMemoryPool();

	std::shared_ptr<GVkBufferView> Emplace(size_t data_size, void* data);
	std::shared_ptr<GVkBufferView> GetBlock(uint32_t block_index, size_t data_size, void* data);

	void Flush();
	void Clear();
};