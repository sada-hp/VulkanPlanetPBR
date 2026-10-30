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

	bDescriptorsDirty = false;
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