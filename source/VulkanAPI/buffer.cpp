#include "pch.hpp"
#include "buffer.hpp"

GVkBuffer::GVkBuffer(std::shared_ptr<RenderScope> InScope, size_t Size, EBufferFlags Flags)
	: Scope(InScope)
	, m_size(Size)
{
	VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, VK_NULL_HANDLE, 0, Size };
	VmaAllocationCreateInfo allocCreateInfo{};

	createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, VK_NULL_HANDLE, 0, Size };
	allocCreateInfo = {};

	if (CheckFlag(Flags, EBufferFlags::Host) || CheckFlag(Flags, EBufferFlags::Mapped))
	{
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

		if (CheckFlag(Flags, EBufferFlags::Mapped))
			allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	else
	{
		// to allow staging buffer updates
		createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	}

	if (CheckFlag(Flags, EBufferFlags::Index))
		createInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	if (CheckFlag(Flags, EBufferFlags::Vertex))
		createInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	if (CheckFlag(Flags, EBufferFlags::Indirect))
		createInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

	if (CheckFlag(Flags, EBufferFlags::Storage))
		createInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	if (CheckFlag(Flags, EBufferFlags::TransferDst))
		createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	if (CheckFlag(Flags, EBufferFlags::Uniform))
		createInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	if (CheckFlag(Flags, EBufferFlags::TransferSrc))
		createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	if (CheckFlag(Flags, EBufferFlags::Concurent))
	{
		createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;


	}
	else
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// TODO : set queues based on usage pattern
	// TODO : move to scope

	//std::vector<uint32_t> queueFamilies = FindDeviceQueues(Scope->GetPhysicalDevice(), { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT });
	//std::sort(queueFamilies.begin(), queueFamilies.end());
	//queueFamilies.resize(std::distance(queueFamilies.begin(), std::unique(queueFamilies.begin(), queueFamilies.end())));

	//createInfo.queueFamilyIndexCount = queueFamilies.size();
	//createInfo.pQueueFamilyIndices = queueFamilies.data();

	if (CheckFlag(Flags, EBufferFlags::InFlight))
		m_flightResources.resize(Scope->GetMaxFramesInFlight());
	else
		m_flightResources.resize(1);

	for (auto& object : m_flightResources)
	{
		auto res = vmaCreateBuffer(Scope->GetAllocator(), &createInfo, &allocCreateInfo, &object.buffer, &object.memory, VK_NULL_HANDLE);
		assert(res == VK_SUCCESS);
	}
}

GVkBuffer::~GVkBuffer()
{
	for (auto& object : m_flightResources)
		vmaDestroyBuffer(Scope->GetAllocator(), object.buffer, object.memory);
}

VmaAllocationInfo GVkBuffer::_allocationInfo() const
{
	auto& object = _activeObj();
	VmaAllocationInfo allocInfo{};
	vmaGetAllocationInfo(Scope->GetAllocator(), object.memory, &allocInfo);

	return allocInfo;
}

VkMemoryPropertyFlags GVkBuffer::_memoryFlags() const
{
	auto& object = _activeObj();
	VkMemoryPropertyFlags memFlags;
	vmaGetAllocationMemoryProperties(Scope->GetAllocator(), object.memory, &memFlags);

	return memFlags;
}

size_t GVkBuffer::GetSize() const
{
	return m_size;
}

const VkBuffer& GVkBuffer::GetBuffer() const
{
	auto& object = _activeObj();
	return object.buffer;
}

void* GVkBuffer::Map()
{
	auto memFlags = _memoryFlags();

	if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		auto allocInfo = _allocationInfo();
		auto& object = _activeObj();

		vmaInvalidateAllocation(Scope->GetAllocator(), object.memory, allocInfo.offset, allocInfo.size);
		vmaMapMemory(Scope->GetAllocator(), object.memory, &allocInfo.pMappedData);

		return allocInfo.pMappedData;
	}

	return nullptr;
}

void GVkBuffer::UnMap()
{
	auto allocInfo = _allocationInfo();

	if (allocInfo.pMappedData)
	{
		auto& object = _activeObj();
		vmaUnmapMemory(Scope->GetAllocator(), object.memory);
		vmaFlushAllocation(Scope->GetAllocator(), object.memory, allocInfo.offset, allocInfo.size);
	}
}

std::shared_ptr<GVkBufferView> GVkBuffer::ToView(std::shared_ptr<GVkBuffer> Buffer, size_t Offset, size_t Size)
{
	return std::make_shared<GVkBufferView>(Buffer, Offset, Size);
}

GVkBufferView::GVkBufferView(std::shared_ptr<GVkBuffer> Buffer, size_t Offset, size_t Size)
	: m_Buffer(Buffer), m_Offset(Offset)
{
	if (Size == VK_WHOLE_SIZE)
	{
		m_Size = Buffer->GetSize() - Offset;
	}
	else
	{
		m_Size = Size;
	}
}