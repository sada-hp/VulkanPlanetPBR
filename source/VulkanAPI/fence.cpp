#include "fence.hpp"

GVkFence::GVkFence(std::shared_ptr<RenderScope> InScope, EFenceFlags Flags)
	: Scope(InScope)
{
	VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceCreateInfo.flags = CheckFlag(Flags, EFenceFlags::CreateSignaled) ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	if (CheckFlag(Flags, EFenceFlags::InFlight))
		m_flightResources.resize(Scope->GetMaxFramesInFlight());
	else
		m_flightResources.resize(1);

	for (auto& object : m_flightResources)
	{
		vkCreateFence(Scope->GetDevice(), &fenceCreateInfo, VK_NULL_HANDLE, &object.fence);
	}
}

GVkFence::~GVkFence()
{
	for (auto& object : m_flightResources)
	{
		if (vkGetFenceStatus(Scope->GetDevice(), object.fence) != VK_SUCCESS)
			vkWaitForFences(Scope->GetDevice(), 1, &object.fence, VK_TRUE, UINT64_MAX);

		vkDestroyFence(Scope->GetDevice(), object.fence, VK_NULL_HANDLE);
	}
}

VkFence GVkFence::GetFence() const
{
	auto& object = _activeObj();
	return object.fence;
}

void GVkFence::Wait() const
{
	auto& object = _activeObj();

	if (vkGetFenceStatus(Scope->GetDevice(), object.fence) != VK_SUCCESS)
		vkWaitForFences(Scope->GetDevice(), 1, &object.fence, VK_TRUE, UINT64_MAX);
}

void GVkFence::Reset()
{
	auto& object = _activeObj();

	if (vkGetFenceStatus(Scope->GetDevice(), object.fence) != VK_SUCCESS)
		vkWaitForFences(Scope->GetDevice(), 1, &object.fence, VK_TRUE, UINT64_MAX);

	vkResetFences(Scope->GetDevice(), 1, &object.fence);
}

void GVkFence::Signal()
{
	auto& object = _activeObj();
	vkQueueSubmit(Scope->GetQueue(VK_QUEUE_GRAPHICS_BIT).GetQueue(), 0, VK_NULL_HANDLE, object.fence);
}