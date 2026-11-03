#pragma once
#include "core.hpp"
#include "scope.hpp"
#include "command_buffer.hpp"
#include "shared_resources.hpp"
#include "FrameGraph/frame_graph.hpp"
#include "RenderStages/IRenderStage.hpp"

class GVulkanBase
{
	friend class GWindow;

protected:
	std::shared_ptr<GFrameGraph> m_FrameGraph = nullptr;
	std::shared_ptr<RenderScope> m_Scope = VK_NULL_HANDLE;
	std::shared_ptr<GVkSwapchain> m_Swapchain = VK_NULL_HANDLE;
	std::shared_ptr<GVkCommandBuffer> m_BlitCommandBuffer = VK_NULL_HANDLE;
	std::shared_ptr<GVkCommandBuffer> m_DrawCommandBuffer = VK_NULL_HANDLE;

protected:
	GVulkanBase(GLFWwindow* window);
	~GVulkanBase();

public:
	bool Draw(const GCamera& Camera, const IWorld& World);
	std::shared_ptr<RenderScope> GetScope() const { return m_Scope; }

	template<typename T, typename ...Args>
	inline void AddRenderStage(Args&& ...args)
	{
		m_FrameGraph->AddStage<T>(m_Scope, args...);
	}
};