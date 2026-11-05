#include "preprocess.hpp"
#include "VulkanAPI/ubo.hpp"
#include "Factories/VkBufferFactory.hpp"
#include "glfw/glfw3.h"

void GPreprocessStage::setup_dependencies(const IWorld& World)
{
	_uboView = GVkBuffer::ToView(GVkSharedResources::Instance.UBO);
	Resources.add(_uboView, EResourceOp::WriteOnly, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

	_stagingBuffer = GVkBufferFactory::Buffer(Scope, EBufferFlags::InFlight | EBufferFlags::TransferSrc | EBufferFlags::Mapped, sizeof(_UniformBuffer));
	_stagingview = GVkBuffer::ToView(_stagingBuffer);

	auto indirect_view = World.GetSceneIndirectBuffer();
	Resources.add(indirect_view, EResourceOp::WriteOnly, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
}

GPreprocessStage::GPreprocessStage(std::shared_ptr<RenderScope> InScope)
	: IRenderStage(InScope, VK_QUEUE_TRANSFER_BIT)
{
}

GPreprocessStage::~GPreprocessStage()
{
}

void GPreprocessStage::record(const GCamera& Camera, const IWorld& World)
{
	static glm::dmat4 reproj = glm::mat4(1.0);

	glm::dvec3 pos = Camera.GetWorldMatrix().GetPosition();
	glm::dmat4 view = Camera.ViewMat4();
	glm::dmat4 world = Camera.WorldMat4();
	glm::dmat4 proj = Camera.ProjectionMat4();
	glm::mat4 basis = glm::mat4(glm::mat3(view));

	_UniformBuffer UBO{};
	UBO.view_proj = proj * view;
	UBO.view_proj_no_offset = glm::mat4(proj) * basis;
	UBO.view_proj_inv = glm::inverse(view) * glm::inverse(proj);
	UBO.reproj = reproj;
	UBO.sun_dir = glm::vec4(World.GetSunDirection(), 0.0);
	UBO.eye_pos = glm::vec4(pos, 1.0);
	UBO.eye_pos_i = glm::ivec4(pos, 0.0);
	UBO.eye_pos_f = glm::vec4(pos - glm::dvec3(UBO.eye_pos_i), 0.0);
	UBO.time = glfwGetTime();

	memcpy(_stagingBuffer->Map(), &UBO, sizeof(UBO));
	_stagingBuffer->UnMap();

	CommandBuffer->CopyBuffer(_stagingview, _uboView);

	auto updateWorld = World.GetPreprocessCommands();

	if (updateWorld != VK_NULL_HANDLE)
		CommandBuffer->Execute(updateWorld);

	reproj = UBO.view_proj;
}