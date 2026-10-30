#include "gpu_culling.hpp"

GCullingStage::GCullingStage(std::shared_ptr<RenderScope> Scope)
	: IRenderStage(Scope, VK_QUEUE_GRAPHICS_BIT)
{
	DescriptorLayoutDescriptor DSLDescIndirect;
	DSLDescIndirect.AddStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT, true);
	auto LayoutIndirect = DSLDescIndirect.Construct(Scope);

	DescriptorLayoutDescriptor DSLDescTransform;
	DSLDescTransform.AddUniformBuffer(VK_SHADER_STAGE_COMPUTE_BIT, true);
	auto LayoutTransform = DSLDescTransform.Construct(Scope);

	ComputePipelineDescriptor CSODesc{};
	CSODesc.AddPushConstants<_shaderData>(VK_SHADER_STAGE_COMPUTE_BIT);
	CSODesc.AddDescriptorLayout(LayoutIndirect->GetLayout());
	CSODesc.AddDescriptorLayout(LayoutTransform->GetLayout());
	CSODesc.CS.AppendCode(GShaders::CullCS);
	Pipeline = CSODesc.Construct(Scope);
}

GCullingStage::~GCullingStage()
{
}

void GCullingStage::Execute(const GCamera& Camera, const IWorld& World)
{
	uint32_t Count = World.GetRegistry().view<GDrawable, EllipsoidMatrix>().size_hint();

	if (Count == 0)
		return;

	uint32_t Invocations = (Count / 32) + uint32_t(Count % 32 > 0);

	_shaderData Data;

	glm::mat4 ViewProjMatrix = Camera.ProjectionViewMat4();
	for (int i = 4; i--; ) { Data.planes[0][i] = ViewProjMatrix[i][3] + ViewProjMatrix[i][0]; }
	for (int i = 4; i--; ) { Data.planes[1][i] = ViewProjMatrix[i][3] - ViewProjMatrix[i][0]; }
	for (int i = 4; i--; ) { Data.planes[2][i] = ViewProjMatrix[i][3] + ViewProjMatrix[i][1]; }
	for (int i = 4; i--; ) { Data.planes[3][i] = ViewProjMatrix[i][3] - ViewProjMatrix[i][1]; }
	for (int i = 4; i--; ) { Data.planes[4][i] = ViewProjMatrix[i][3] + ViewProjMatrix[i][2]; }
	for (int i = 4; i--; ) { Data.planes[5][i] = ViewProjMatrix[i][3] - ViewProjMatrix[i][2]; }

	Data.invocations = Count;

	CommandBuffer->BindPipeline(Pipeline);
	CommandBuffer->PushConstants(0, Data);
	CommandBuffer->BindDescriptorSet(0, World.GetSceneIndirectSet());
	CommandBuffer->BindDescriptorSet(1, World.GetSceneTransformsSet());
	CommandBuffer->Dispatch(Invocations, 1, 1);
	CommandBuffer->Submit();
}