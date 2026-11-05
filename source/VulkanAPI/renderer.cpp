#include "pch.hpp"
#include "ubo.hpp"
#include "glfw/glfw3.h"
#include "renderer.hpp"
#include "Factories/VkImageFactory.hpp"
#include "Factories/VkBufferFactory.hpp"

#include "RenderStages/Precompute/atmosphereLUT.hpp"
#include "RenderStages/mesh_objects.hpp"
#include "RenderStages/gpu_culling.hpp"
#include "RenderStages/tonemapping.hpp"
#include "RenderStages/atmosphere.hpp"
#include "RenderStages/clear_pass.hpp"
#include "RenderStages/preprocess.hpp"
#include "RenderStages/clouds.hpp"

GVulkanBase::GVulkanBase(GLFWwindow* window)
{
	m_Scope = std::make_shared<RenderScope>();
	m_FrameGraph = std::make_shared<GFrameGraph>(m_Scope);

	m_Swapchain = std::make_shared<GVkSwapchain>(m_Scope, window);
	m_BlitCommandBuffer = std::make_shared<GVkCommandBuffer>(m_Scope, VK_QUEUE_GRAPHICS_BIT, ECmdFlags::InFlight);
	m_DrawCommandBuffer = std::make_shared<GVkCommandBuffer>(m_Scope, VK_QUEUE_GRAPHICS_BIT, ECmdFlags::InFlight);

	GVkSharedResources::Instance.FinalTarget = GVkImageFactory::FinalRenderTargetFlight(m_Scope, m_Swapchain->GetExtent());
	GVkSharedResources::Instance.ColorBuffer = GVkImageFactory::ColorRenderTargetFlight(m_Scope, m_Swapchain->GetExtent());
	GVkSharedResources::Instance.DepthBuffer = GVkImageFactory::DepthRenderTargetFlight(m_Scope, m_Swapchain->GetExtent());
	GVkSharedResources::Instance.UBO = GVkBufferFactory::Buffer(m_Scope, EBufferFlags::InFlight | EBufferFlags::Uniform, sizeof(_UniformBuffer));

	// precompute
	GAtmospherePrecomputeLUT::Execute(m_Scope);

	// render stages
	AddRenderStage<GPreprocessStage>();
	AddRenderStage<GCloudsStage>();
	AddRenderStage<GCullingStage>();
	AddRenderStage<GMeshStage>();
	AddRenderStage<GAtmosphereStage>();
	AddRenderStage<GTonemapStage>();
	AddRenderStage<GAttachmentsClear>();
}

GVulkanBase::~GVulkanBase()
{
}

bool GVulkanBase::Draw(const GCamera& Camera, const IWorld& World)
{
	if (m_Swapchain->QueryNextImage())
	{
		float Aspect = static_cast<float>(m_Swapchain->GetExtent().width) / static_cast<float>(m_Swapchain->GetExtent().height);
		const_cast<GCamera&>(Camera).GetProjectionMatrix().SetAspect(Aspect);

		m_FrameGraph->Draw(Camera, World);

		m_BlitCommandBuffer->BlitImage(GVkImage::ToView(GVkSharedResources::Instance.FinalTarget), m_Swapchain);
		m_BlitCommandBuffer->Submit();
		m_Swapchain->Present();

		m_Scope->IncrementFlightIndex();
		return true;
	}

	return false;
}