#pragma once
#include "scope.hpp"

enum class EFenceFlags
{
	None = 0,
	CreateSignaled = Bit(0),
	InFlight       = Bit(1)
};

DefineFlags(EFenceFlags)

class GVkFence : public IVkObj
{
	struct _internalObj
	{
		VkFence fence = VK_NULL_HANDLE;
	};

private:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	std::vector<_internalObj> m_flightResources = {};

private:
	size_t _activeIndex() const { return Scope->GetResourceIndex() % m_flightResources.size(); }
	const _internalObj& _activeObj() const { return m_flightResources.at(_activeIndex()); }
	_internalObj& _activeObj() { return m_flightResources.at(_activeIndex()); }

public:
	inline const VkFence& operator[](size_t index) const { return m_flightResources.at(index % m_flightResources.size()).fence; }

public:
	GVkFence(std::shared_ptr<RenderScope> Scope, EFenceFlags Flags);
	virtual ~GVkFence();

	bool IsInFlight() const { return m_flightResources.size() != 1; }
	const VkFence& At(size_t Index) const { return (*this)[Index]; }

	VkFence GetFence() const;
	void Wait() const;
	void Reset();

	void Signal();
};