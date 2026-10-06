#pragma once
#include "scope.hpp"

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

class GVkBufferView;

class GVkBuffer : public IVkObj
{
	friend class GVkBufferView;

	struct _internalObj
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation memory = VK_NULL_HANDLE;
	};

private:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	std::vector<_internalObj> m_flightResources = {};
	size_t m_size = 0;

private:
	VmaAllocationInfo _allocationInfo() const;
	VkMemoryPropertyFlags _memoryFlags() const;

private:
	size_t _activeIndex() const { return Scope->GetResourceIndex() % m_flightResources.size(); }
	const _internalObj& _activeObj() const { return m_flightResources.at(_activeIndex()); }
	_internalObj& _activeObj() { return m_flightResources.at(_activeIndex()); }

public:
	inline const VkBuffer& operator[](size_t index) const { return m_flightResources.at(index % m_flightResources.size()).buffer; }

public:
	GVkBuffer(std::shared_ptr<RenderScope> Scope, size_t Size, EBufferFlags Flags);
	virtual ~GVkBuffer();

	bool IsInFlight() const { return m_flightResources.size() != 1; }
	const VkBuffer& At(size_t Index) const { return (*this)[Index]; }

	size_t GetSize() const;
	const VkBuffer& GetBuffer() const;

	void* Map();
	void UnMap();

	static std::shared_ptr<GVkBufferView> ToView(std::shared_ptr<GVkBuffer> Buffer, size_t Offset = 0, size_t Size = VK_WHOLE_SIZE);
};

class GVkBufferView : public IVkObj
{
private:
	std::shared_ptr<GVkBuffer> m_Buffer = VK_NULL_HANDLE;
	size_t m_Offset = 0u;
	size_t m_Size = 0u;

public:
	GVkBufferView(std::shared_ptr<GVkBuffer> Buffer, size_t Offset = 0, size_t Size = VK_WHOLE_SIZE);

	const VkBuffer& At(size_t index) const { return m_Buffer->At(index); }
	const VkBuffer& GetBuffer() const { return m_Buffer->GetBuffer(); }
	size_t GetOffset() const { return m_Offset; }
	size_t GetSize() const { return m_Size; }

	std::shared_ptr<GVkBuffer> GetRoot() const { return m_Buffer; }
};