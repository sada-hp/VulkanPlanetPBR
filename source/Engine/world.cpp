#include "world.hpp"

IWorld::IWorld(std::shared_ptr<RenderScope> InScope)
	: Scope(InScope)
{
	registry.on_construct<GDrawable>().connect<&IWorld::register_gpu_actor>(this);
	registry.on_destroy<GDrawable>().connect<&IWorld::remove_gpu_actor>(this);
	registry.on_update<GDrawable>().connect<&IWorld::update_gpu_actor>(this);
}

IWorld::~IWorld()
{
}

void IWorld::register_gpu_actor(entt::registry& reg, entt::entity ent)
{
	registry.get<EllipsoidMatrix>(ent).InitBufferGPU(Scope);
	bDescriptorsDirty = true;
}

void IWorld::remove_gpu_actor(entt::registry& reg, entt::entity ent)
{
	bDescriptorsDirty = true;
}

void IWorld::update_gpu_actor(entt::registry& reg, entt::entity ent)
{
	bDescriptorsDirty = true;
}

void IWorld::update_descriptors()
{
	auto view = registry.view<EllipsoidMatrix, GDrawable>();
	
	std::vector<std::shared_ptr<GVkBufferView>> transformBuffers;
	transformBuffers.reserve(view.size_hint());

	std::vector<std::shared_ptr<GVkBufferView>> indirectBuffers;
	indirectBuffers.reserve(view.size_hint());

	for (const auto& [ent, matrix, draw] : view.each())
	{
		indirectBuffers.emplace_back(draw.GetIndirectData()->IndirectBuffer());
		transformBuffers.emplace_back(matrix.GetBufferGPU());
	}

	DescriptorSetDescriptor DSDescIndirect;
	DSDescIndirect.AddStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT, indirectBuffers);
	_indirectSet = DSDescIndirect.Allocate(Scope);

	DescriptorSetDescriptor DSDescTransform;
	DSDescTransform.AddUniformBuffer(VK_SHADER_STAGE_COMPUTE_BIT, transformBuffers);
	_transformsSet = DSDescTransform.Allocate(Scope);

	_syncCommandBuffer = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_GRAPHICS_BIT, ECmdFlags::InFlight | ECmdFlags::Secondary);

	bDescriptorsDirty = false;
}

VkCommandBuffer IWorld::GetPrerecorderSyncCommands() const
{
	if (!_syncCommandBuffer)
		return VK_NULL_HANDLE;

	if (!_syncCommandBuffer->IsRecorded())
	{
		auto view = registry.view<EllipsoidMatrix, GDrawable>();

		std::vector<VkBufferMemoryBarrier2> bufferBarriers{};
		for (const auto& [ent, matrix, object] : view.each())
		{
			GVkBarrier barrierConditional = GVkBarrier(object._drawCall->ConditionalBuffer(), VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT, VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT);
			barrierConditional.GetBufferBarreirs(bufferBarriers);

			GVkBarrier barrierIndirect = GVkBarrier(object._drawCall->IndirectBuffer(), VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
			barrierIndirect.GetBufferBarreirs(bufferBarriers);
		}

		_syncCommandBuffer->BindBarriers(bufferBarriers, {});
		_syncCommandBuffer->Submit();
	}

	return _syncCommandBuffer->GetCommandBuffer();
}

std::shared_ptr<GActor> IWorld::NewActor()
{
	return std::make_shared<GActor>(registry);
}

std::shared_ptr<GVkDescriptorSet> IWorld::GetSceneTransformsSet() const
{
	if (bDescriptorsDirty)
		const_cast<IWorld*>(this)->update_descriptors();

	return _transformsSet;
}

std::shared_ptr<GVkDescriptorSet> IWorld::GetSceneIndirectSet() const
{
	if (bDescriptorsDirty)
		const_cast<IWorld*>(this)->update_descriptors();

	return _indirectSet;
}