#include "pch.hpp"
#include "glfw/glfw3.h"
#include "renderer.hpp"
#include "Factories/VkImageFactory.hpp"
#include "Factories/VkBufferFactory.hpp"

#include "RenderStages/Precompute/atmosphereLUT.hpp"
#include "RenderStages/mesh_objects.hpp"
#include "RenderStages/atmosphere.hpp"
#include "RenderStages/clouds.hpp"

struct _UniformBuffer
{
	glm::mat4 view_proj;
	glm::mat4 view_proj_inv;

	glm::vec4 sun_dir;
	glm::vec4 eye_pos;
};

GVulkanBase::GVulkanBase(GLFWwindow* window)
{
	m_Scope = std::make_shared<RenderScope>();
	m_Swapchain = std::make_shared<GVkSwapchain>(m_Scope, window);
	m_BlitCommandBuffer = std::make_shared<GVkCommandBuffer>(m_Scope, VK_QUEUE_GRAPHICS_BIT, ECmdFlags::InFlight);
	m_DrawCommandBuffer = std::make_shared<GVkCommandBuffer>(m_Scope, VK_QUEUE_GRAPHICS_BIT, ECmdFlags::InFlight);

	m_Resources.ColorBuffer = GVkImageFactory::ColorRenderTargetFlight(m_Scope, m_Swapchain->GetExtent());
	m_Resources.UBO = GVkBufferFactory::Buffer(m_Scope, EBufferFlags::InFlight | EBufferFlags::Mapped | EBufferFlags::Uniform, sizeof(_UniformBuffer));

	// precompute
	GAtmospherePrecomputeLUT::Execute(m_Scope, m_Resources);

	// render stages
	m_RenderStages.emplace_back(new GAtmosphereStage(m_Scope, m_Resources));
	// m_RenderStages.emplace_back(new GCloudsStage(m_Scope, m_Resources));
	m_RenderStages.emplace_back(new GMeshStage(m_Scope, m_Resources));
}

GVulkanBase::~GVulkanBase()
{
	for (auto& Stage : m_RenderStages)
		delete Stage;
}

bool GVulkanBase::Draw(const GCamera& Camera, const IWorld& World)
{
	if (m_Swapchain->QueryNextImage())
	{
		float Aspect = static_cast<float>(m_Swapchain->GetExtent().width) / static_cast<float>(m_Swapchain->GetExtent().height);
		const_cast<GCamera&>(Camera).GetProjectionMatrix().SetAspect(Aspect);

		glm::dmat4 view  = Camera.ViewMat4();
		glm::dmat4 world = Camera.WorldMat4();
		glm::dmat4 proj  = Camera.ProjectionMat4();

		_UniformBuffer UBO{};
		UBO.view_proj = proj * view;
		UBO.view_proj_inv = glm::inverse(view) * glm::inverse(proj);
		UBO.sun_dir = glm::vec4(glm::normalize(glm::vec3(1.0)), 0.0);
		UBO.eye_pos = glm::vec4(Camera.GetWorldMatrix().GetPosition(), 1.0);

		memcpy(m_Resources.UBO->Map(), &UBO, sizeof(UBO));
		m_Resources.UBO->UnMap();

		for (auto& Stage : m_RenderStages)
			Stage->Execute(Camera, World);

		m_BlitCommandBuffer->BlitImage(GVkImage::ToView(m_Resources.ColorBuffer), m_Swapchain);
		m_BlitCommandBuffer->Submit();
		m_Swapchain->Present();

		m_Scope->IncrementFlightIndex();

		return true;
	}

	return false;
}