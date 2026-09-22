#include "pch.hpp"
#include "buffer.hpp"

static void _flags_to_info(const RenderScope& Scope, EBufferFlags Flags, size_t Size, VkBufferCreateInfo& createInfo, VmaAllocationCreateInfo& allocCreateInfo)
{
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
		createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	else
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// TODO : set queues based on usage pattern
	// TODO : move to scope

	std::vector<uint32_t> queueFamilies = FindDeviceQueues(Scope.GetPhysicalDevice(), { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT });
	std::sort(queueFamilies.begin(), queueFamilies.end());
	queueFamilies.resize(std::distance(queueFamilies.begin(), std::unique(queueFamilies.begin(), queueFamilies.end())));

	createInfo.queueFamilyIndexCount = queueFamilies.size();
	createInfo.pQueueFamilyIndices = queueFamilies.data();
}

GVkBuffer::GVkBuffer(const RenderScope& InScope, size_t Size, EBufferFlags Flags)
	: Scope(&InScope)
{
	VkBufferCreateInfo createInfo{};
	VmaAllocationCreateInfo allocCreateInfo{};
	_flags_to_info(InScope, Flags, Size, createInfo, allocCreateInfo);

	m_flightResources.resize(1);
	for (auto& object : m_flightResources)
	{
		assert(vmaCreateBuffer(Scope->GetAllocator(), &createInfo, &allocCreateInfo, &object.buffer, &object.memory, nullptr) == VK_SUCCESS);

		object.descriptorInfo.buffer = object.buffer;
		object.descriptorInfo.range = Size;
		object.descriptorInfo.offset = 0;
	}
}

GVkBuffer::GVkBuffer(GVkBuffer&& other) noexcept
	: Scope(other.Scope), m_flightResources(std::move(other.m_flightResources))
{
	other.m_flightResources.clear();
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

void GVkBuffer::_clamp_range(size_t& size, size_t& offset) const
{
	auto allocInfo = _allocationInfo();
	size = std::min(size, allocInfo.size);

	assert(offset + size <= allocInfo.size);
	assert(size > 0);
}

size_t GVkBuffer::GetSize() const
{
	auto allocInfo = _allocationInfo();
	return allocInfo.size;
}

const VkBuffer& GVkBuffer::GetBuffer() const
{
	auto& object = _activeObj();
	return object.buffer;
}

const VkDescriptorBufferInfo& GVkBuffer::GetDescriptor() const
{
	auto& object = _activeObj();
	return object.descriptorInfo;
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

GVkBuffer& GVkBuffer::Update(void* data, size_t data_size, size_t offset)
{
	_clamp_range(data_size, offset);

	if (char* mappedMemory = (char*)Map())
	{
		memcpy(mappedMemory + offset, data, data_size);
		UnMap();
	}
	else
	{
		auto& object = _activeObj();

		auto stageBuffer = std::make_shared<GVkBuffer>(*Scope, data_size, EBufferFlags::Mapped | EBufferFlags::TransferSrc);
		stageBuffer->Update(data, data_size);

		VkCommandBuffer cmd;
		Scope->GetQueue(VK_QUEUE_TRANSFER_BIT)
			.AllocateCommandBuffers(1, &cmd);

		::BeginOneTimeSubmitCmd(cmd);

		VkBufferCopy region{};
		region.dstOffset = offset;
		region.size = data_size;
		region.srcOffset = 0;

		vkCmdCopyBuffer(cmd, stageBuffer->GetBuffer(), object.buffer, 1, &region);

		::EndCommandBuffer(cmd);
		Scope->GetQueue(VK_QUEUE_TRANSFER_BIT)
			.Submit(cmd)
			.Wait()
			.FreeCommandBuffers(1, &cmd);
	}

	return *this;
}

GVkBuffer& GVkBuffer::Update(VkCommandBuffer cmd, void* data, size_t data_size, size_t offset)
{
	_clamp_range(data_size, offset);

	auto& object = _activeObj();
	vkCmdUpdateBuffer(cmd, object.buffer, offset, data_size, data);

	return *this;
}