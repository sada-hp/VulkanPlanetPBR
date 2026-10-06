#pragma once
#include "VulkanAPI/command_buffer.hpp"
#include "Engine/camera.hpp"
#include "Engine/world.hpp"

class IRenderStage
{
protected:
	std::shared_ptr<GVkCommandBuffer> CommandBuffer = VK_NULL_HANDLE;
	std::shared_ptr<GVkFramebuffer>   Framebuffer   = VK_NULL_HANDLE;
	std::shared_ptr<GVkRenderPass>    RenderPass    = VK_NULL_HANDLE;
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;

public:
	IRenderStage(std::shared_ptr<RenderScope> InScope, VkQueueFlagBits Type)
		: Scope(InScope)
	{
		CommandBuffer = std::make_shared<GVkCommandBuffer>(Scope, Type, ECmdFlags::InFlight);
	}

	virtual ~IRenderStage() {};

	virtual void Execute(const GCamera& Camera, const IWorld& World) = 0;
	virtual bool IsActive() { return true; }
};