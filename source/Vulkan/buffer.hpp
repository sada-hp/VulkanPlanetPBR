#pragma once
#include <array>
#include <vector>
#include <glfw/glfw3.h>
#include "Vulkan/scope.hpp"
#include <vma/vk_mem_alloc.h>

enum class EBufferFlags
{
	Uniform     = Bit(1),
	Storage     = Bit(2),
	Indirect    = Bit(3),
	Index       = Bit(4),
	Vertex      = Bit(5),
	Mapped      = Bit(6),
	Host        = Bit(7),
	TransferDst = Bit(8),
	TransferSrc = Bit(9),
	InFlight    = Bit(10),
	Concurent   = Bit(11)
};

DefineFlags(EBufferFlags)

class GVkBuffer
{
	struct _internalObj
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation memory = VK_NULL_HANDLE;
		VkDescriptorBufferInfo descriptorInfo = {};
	};

private:
	std::vector<_internalObj> m_flightResources = {};
	// TODO : make shared
	const RenderScope* Scope = VK_NULL_HANDLE;

private:
	VmaAllocationInfo _allocationInfo() const;
	VkMemoryPropertyFlags _memoryFlags() const;
	void _clamp_range(size_t& size, size_t& offset) const;

	size_t _activeIndex() const { return m_flightResources.size() == 1 ? 0 : Scope->GetMaxFramesInFlight(); }
	const _internalObj& _activeObj() const { return m_flightResources.at(_activeIndex()); }
	_internalObj& _activeObj() { return m_flightResources.at(_activeIndex()); }

public:
	inline void operator=(const GVkBuffer& other) = delete;
	inline const VkBuffer& operator[](size_t index) const { return m_flightResources.at(index % m_flightResources.size()).buffer; }

public:
	GVkBuffer(const RenderScope& Scope, size_t Size, EBufferFlags Flags);
	GVkBuffer(const GVkBuffer& other) = delete;
	GVkBuffer(GVkBuffer&& other) noexcept;
	~GVkBuffer();

	bool IsInFlight() const { return m_flightResources.size() != 1; }
	const VkBuffer& At(size_t Index) const { return (*this)[Index]; }

	size_t GetSize() const;
	const VkBuffer& GetBuffer() const;
	const VkDescriptorBufferInfo& GetDescriptor() const;

	void* Map();
	void UnMap();

	GVkBuffer& Update(void* data, size_t data_size = VK_WHOLE_SIZE, size_t offset = 0);
	GVkBuffer& Update(VkCommandBuffer cmd, void* data, size_t data_size = VK_WHOLE_SIZE, size_t offset = 0);
};