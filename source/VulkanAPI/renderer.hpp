#pragma once
#include "core.hpp"
#include "scope.hpp"
#include "command_buffer.hpp"
#include "shared_resources.hpp"
#include "RenderStages/IRenderStage.hpp"

struct GFrameData
{
	GCamera& Camera;

	size_t count = 0;
	GDrawable* objects = VK_NULL_HANDLE;
};

class GVulkanBase
{
	friend class GWindow;

private:
	std::shared_ptr<RenderScope> m_Scope = VK_NULL_HANDLE;
	std::shared_ptr<GVkSwapchain> m_Swapchain = VK_NULL_HANDLE;
	std::shared_ptr<GVkCommandBuffer> m_BlitCommandBuffer = VK_NULL_HANDLE;
	std::shared_ptr<GVkCommandBuffer> m_DrawCommandBuffer = VK_NULL_HANDLE;

protected:
	GVkSharedResources m_Resources;
	std::vector<IRenderStage*> m_RenderStages = {};

protected:
	GVulkanBase(GLFWwindow* window);
	~GVulkanBase();

public:
	bool Draw(const GCamera& Camera, const IWorld& World);
	std::shared_ptr<RenderScope> GetScope() const { return m_Scope; }
};