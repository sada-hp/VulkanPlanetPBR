#include "gpu_block.hpp"
#include "Factories/VkBufferFactory.hpp"

GGPuMemoryPool::GGPuMemoryPool(std::shared_ptr<RenderScope> InScope, EBufferFlags flags, size_t stride, size_t count)
	: Scope(InScope)
	, m_Stride(stride)
	, m_Flags(flags)
{
	_views.resize(count);
	m_Buffer = GVkBufferFactory::Buffer(Scope, flags, stride * (count + 1));
	_commandBuffer = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_TRANSFER_BIT);
}

GGPuMemoryPool::~GGPuMemoryPool()
{
}

size_t GGPuMemoryPool::_findSuitableIndex()
{
	size_t index = 0;

	for (auto it = _views.begin(); it != _views.end(); it++)
	{
		if (*it == nullptr || it->use_count() == 1)
			break;

		index++;
	}

	return index;
}

std::shared_ptr<GVkBufferView> GGPuMemoryPool::Emplace(size_t data_size, void* data)
{
	return GetBlock(_findSuitableIndex(), data_size, data);
}

std::shared_ptr<GVkBufferView> GGPuMemoryPool::GetBlock(uint32_t block_index, size_t data_size, void* data)
{
	assert(data_size < m_Stride);

	size_t offset = block_index * m_Stride;
	_views[block_index] = GVkBuffer::ToView(m_Buffer, offset, data_size);
	_commandBuffer->UpdateBuffer(_views[block_index], (char*)data);

	return _views[block_index];
}

void GGPuMemoryPool::Flush()
{
	_commandBuffer->Submit();
	_commandBuffer->Reset();
}

void GGPuMemoryPool::Clear()
{
	size_t count = _views.size();
	_views.resize(0); _views.resize(count);

	_commandBuffer = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_TRANSFER_BIT);
}