#include "gpu_culling.hpp"

void GCullingStage::setup_dependencies(const IWorld& World)
{
	auto indirect_view = World.GetSceneIndirectBuffer();
	Resources.add(indirect_view, EResourceOp::WriteOnly, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT);
}

GCullingStage::GCullingStage(std::shared_ptr<RenderScope> Scope)
	: IRenderStage(Scope, VK_QUEUE_COMPUTE_BIT)
{
	DescriptorLayoutDescriptor DSLDescIndirect;
	DSLDescIndirect.AddStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT);
	auto LayoutIndirect = DSLDescIndirect.Construct(Scope);

	DescriptorLayoutDescriptor DSLDescTransform;
	DSLDescTransform.AddStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT);
	auto LayoutTransform = DSLDescTransform.Construct(Scope);

	ComputePipelineDescriptor CSODesc{};
	CSODesc.AddPushConstants<_shaderData>(VK_SHADER_STAGE_COMPUTE_BIT);
	CSODesc.AddDescriptorLayout(LayoutIndirect);
	CSODesc.AddDescriptorLayout(LayoutTransform);
	CSODesc.CS.AppendCode(GShaderUtils::UtilsCommon)
		.AppendCode(GShaders::CullCS);

	Pipeline = CSODesc.Construct(Scope);
}

GCullingStage::~GCullingStage()
{
}

bool GCullingStage::IsActive(const IWorld& World)
{
	auto storage = World.GetRegistry().storage<GDrawable>();

	if (storage && !storage->empty())
		return true;

	return false;
}

void GCullingStage::record(const GCamera& Camera, const IWorld& World)
{
	uint32_t Count = World.GetRegistry().view<GDrawable, EllipsoidMatrix>().size_hint();
	uint32_t Invocations = (Count / 32) + uint32_t(Count % 32 > 0);

	_shaderData Data{};

	glm::dmat4 m = glm::transpose(Camera.ProjectionViewMat4());
	Data.planes[0] = m[3] + m[0];
	Data.planes[1] = m[3] - m[0];
	Data.planes[2] = m[3] + m[1];
	Data.planes[3] = m[3] - m[1];
	Data.planes[4] = m[2];
	Data.planes[5] = m[3] - m[2];

	for (int i = 0; i < 6; i++)
		Data.planes[i] /= glm::length(glm::vec3(Data.planes[i]));

	Data.invocations = Count;

	CommandBuffer->BindPipeline(Pipeline);
	CommandBuffer->PushConstants(0, Data);
	CommandBuffer->BindDescriptorSet(0, World.GetSceneIndirectSet());
	CommandBuffer->BindDescriptorSet(1, World.GetSceneTransformsSet());
	CommandBuffer->Dispatch(Invocations, 1, 1);
}