#pragma once
#include "scope.hpp"

enum class ESemaphoreFlags
{
	None = 0,
	InFlight = Bit(0)
};

DefineFlags(ESemaphoreFlags)

class GVkSemaphore : public IVkObj
{
	struct _internalObj
	{
		VkSemaphore semaphore = VK_NULL_HANDLE;
		mutable bool active = false;
	};

private:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	std::vector<_internalObj> m_flightResources = {};
	VkPipelineStageFlags m_StageFlags;

private:
	size_t _activeIndex() const { return Scope->GetResourceIndex() % m_flightResources.size(); }
	const _internalObj& _activeObj() const { return m_flightResources.at(_activeIndex()); }
	_internalObj& _activeObj() { return m_flightResources.at(_activeIndex()); }

public:
	inline const VkSemaphore& operator[](size_t index) const { return m_flightResources.at(index % m_flightResources.size()).semaphore; }

public:
	GVkSemaphore(std::shared_ptr<RenderScope> Scope, VkPipelineStageFlags Stage, ESemaphoreFlags Flags);
	virtual ~GVkSemaphore();

	bool IsInFlight() const { return m_flightResources.size() != 1; }
	const VkSemaphore& At(size_t Index) const { return (*this)[Index]; }

	VkPipelineStageFlags GetPipelineStage() const;

	VkSemaphore GetSignalSemaphore() const;
	VkSemaphore GetWaitSemaphore() const;
};