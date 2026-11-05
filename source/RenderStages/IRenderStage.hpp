#pragma once
#include "VulkanAPI/shared_resources.hpp"
#include "VulkanAPI/command_buffer.hpp"
#include "Containers/stage_views.hpp"
#include "Engine/camera.hpp"
#include "Engine/world.hpp"

class IRenderStage
{
	friend class GFrameGraph;

protected:
	std::shared_ptr<GVkCommandBuffer> CommandBuffer = VK_NULL_HANDLE;
	std::shared_ptr<GVkFramebuffer>   Framebuffer   = VK_NULL_HANDLE;
	std::shared_ptr<GVkRenderPass>    RenderPass    = VK_NULL_HANDLE;
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	
	GStageViews Resources;
	VkQueueFlagBits Type;

private:
	void _acquire_resources(const std::vector<std::shared_ptr<GVkBarrier>>& ownership)
	{
		std::vector<VkImageMemoryBarrier2> images;
		images.reserve(Resources.images_size() * 2);

		std::vector<VkBufferMemoryBarrier2> buffers;
		buffers.reserve(Resources.buffers_size() * 2);

		for (auto& barrier : ownership)
		{
			barrier->GetBufferBarriers(buffers, EOwnershipType::Acquire);
			barrier->GetImageBarriers(images, EOwnershipType::Acquire);
		}

		for (auto it = Resources.image_begin(); it != Resources.image_end(); it++)
		{
			GVkBarrier barrier(it->view, it->layout, it->stage, it->access);
			barrier.GetImageBarriers(images);
		}

		for (auto it = Resources.buffer_begin(); it != Resources.buffer_end(); it++)
		{
			GVkBarrier barrier(it->view, it->stage, it->access);
			barrier.GetImageBarriers(images);
		}

		CommandBuffer->BindBarriers(buffers, images);
	}

	void _submit(const std::vector<std::shared_ptr<GVkBarrier>>& ownership, const std::vector<std::shared_ptr<GVkSemaphore>>& waitSemaphores, const std::vector<std::shared_ptr<GVkSemaphore>>& signalSemaphores)
	{
		assert(CommandBuffer->IsRecording());

		std::vector<VkImageMemoryBarrier2> images;
		images.reserve(Resources.images_size());

		std::vector<VkBufferMemoryBarrier2> buffers;
		buffers.reserve(Resources.buffers_size());

		for (auto& barrier : ownership)
		{
			barrier->GetBufferBarriers(buffers, EOwnershipType::Release);
			barrier->GetImageBarriers(images, EOwnershipType::Release);
		}

		if (!buffers.empty() || !images.empty())
		{
			CommandBuffer->BindBarriers(buffers, images);
		}

		CommandBuffer->Submit(waitSemaphores, signalSemaphores);
	}

	bool _dependsOn(const IRenderStage& other)
	{
		for (auto it = Resources.image_cbegin(); it != Resources.image_cend(); it++)
		{
			auto oit = other.Resources.find(it->view);

			if (oit != other.Resources.image_cend())
				return static_cast<int>(oit->Op) < static_cast<int>(it->Op);
		}

		for (auto it = Resources.buffer_cbegin(); it != Resources.buffer_cend(); it++)
		{
			auto oit = other.Resources.find(it->view);

			if (oit != other.Resources.buffer_cend())
				return static_cast<int>(oit->Op) < static_cast<int>(it->Op);
		}

		return false;
	}

protected:
	virtual void setup_dependencies(const IWorld& World) = 0;
	virtual void record(const GCamera& Camera, const IWorld& World) = 0;

public:
	IRenderStage(std::shared_ptr<RenderScope> InScope, VkQueueFlagBits InType)
		: Scope(InScope)
	{
		CommandBuffer = std::make_shared<GVkCommandBuffer>(Scope, InType, ECmdFlags::InFlight);
		Type = CommandBuffer->GetType();
	}

	virtual ~IRenderStage() {};

	void Reset(const IWorld& World)
	{
		CommandBuffer = std::make_shared<GVkCommandBuffer>(Scope, Type, ECmdFlags::InFlight);

		Resources.clear();
		setup_dependencies(World);
	}

	virtual bool IsActive(const IWorld& World) { return true; }
};