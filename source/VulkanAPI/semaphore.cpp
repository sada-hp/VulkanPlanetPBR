#include "semaphore.hpp"

GVkSemaphore::GVkSemaphore(std::shared_ptr<RenderScope> InScope, VkPipelineStageFlags Stage, ESemaphoreFlags Flags)
	: Scope(InScope)
	, m_StageFlags(Stage)
{
	m_flightResources.resize(CheckFlag(Flags, ESemaphoreFlags::InFlight) ? Scope->GetMaxFramesInFlight() : 1);

	for (auto& object : m_flightResources)
	{
		VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		vkCreateSemaphore(Scope->GetDevice(), &createInfo, VK_NULL_HANDLE, &object.semaphore);
	}
}

GVkSemaphore::~GVkSemaphore()
{
	for (auto& object : m_flightResources)
		vkDestroySemaphore(Scope->GetDevice(), object.semaphore, VK_NULL_HANDLE);
}

VkPipelineStageFlags GVkSemaphore::GetPipelineStage() const
{
	return m_StageFlags;
}

VkSemaphore GVkSemaphore::GetSignalSemaphore() const
{
	auto& object = _activeObj();
	object.active = true;

	return object.semaphore;
}

VkSemaphore GVkSemaphore::GetWaitSemaphore() const
{
	auto& object = _activeObj();

	if (object.active)
	{
		object.active = false;
		return object.semaphore;
	}

	return VK_NULL_HANDLE;
}
