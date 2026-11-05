#include "world.hpp"

struct SMeshData
{
	DecomposedMatrix transform;
	glm::vec3 min;
	glm::vec3 max;
};

IWorld::IWorld(std::shared_ptr<RenderScope> InScope)
	: Scope(InScope)
{
	registry.on_construct<GDrawable>().connect<&IWorld::register_gpu_actor>(this);
	registry.on_destroy<GDrawable>().connect<&IWorld::remove_gpu_actor>(this);
	registry.on_update<GDrawable>().connect<&IWorld::update_gpu_actor>(this);

	_indirectPool = GVkBufferFactory::Buffer(Scope, EBufferFlags::Storage | EBufferFlags::Conditional | EBufferFlags::Indirect | EBufferFlags::InFlight, sizeof(VkDrawIndexedIndirectCommand) * 16192);
	
	DescriptorSetDescriptor DSDescIndirect;
	DSDescIndirect.AddStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT, GVkBuffer::ToView(_indirectPool));
	_indirectSet = DSDescIndirect.Allocate(Scope);

	_transformsPool = GVkBufferFactory::Buffer(Scope, EBufferFlags::Storage | EBufferFlags::Host | EBufferFlags::Vertex, sizeof(SMeshData) * 16192);

	DescriptorSetDescriptor DSDescTransform;
	DSDescTransform.AddStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT, GVkBuffer::ToView(_transformsPool));
	_transformsSet = DSDescTransform.Allocate(Scope);
}

IWorld::~IWorld()
{
}

void IWorld::register_gpu_actor(entt::registry& reg, entt::entity ent)
{
	bDescriptorsDirty = true;
	_drawableRevision++;
}

void IWorld::remove_gpu_actor(entt::registry& reg, entt::entity ent)
{
	bDescriptorsDirty = true;
	_drawableRevision++;
}

void IWorld::update_gpu_actor(entt::registry& reg, entt::entity ent)
{
	bDescriptorsDirty = true;
	_drawableRevision++;
}

void IWorld::update_descriptors()
{
	auto view = registry.view<EllipsoidMatrix, GDrawable>();
	size_t objects_count = view.size_hint();

	size_t t_size = sizeof(SMeshData) * objects_count;
	if (t_size > _transformsPool->GetSize())
	{
		_transformsPool = GVkBufferFactory::Buffer(Scope, EBufferFlags::Storage | EBufferFlags::Host | EBufferFlags::Vertex, t_size);

		DescriptorSetDescriptor DSDescTransform;
		DSDescTransform.AddStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT, GVkBuffer::ToView(_transformsPool));
		_transformsSet = DSDescTransform.Allocate(Scope);
	}

	size_t c_size = sizeof(VkDrawIndexedIndirectCommand) * objects_count;
	if (c_size > _indirectPool->GetSize())
	{
		_indirectPool = GVkBufferFactory::Buffer(Scope, EBufferFlags::Storage | EBufferFlags::Conditional | EBufferFlags::Indirect | EBufferFlags::InFlight, c_size);

		DescriptorSetDescriptor DSDescIndirect;
		DSDescIndirect.AddStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT, GVkBuffer::ToView(_indirectPool));
		_indirectSet = DSDescIndirect.Allocate(Scope);
	}

	std::vector<VkDrawIndexedIndirectCommand> drawCommands{};
	drawCommands.reserve(objects_count);

	std::vector<SMeshData> sceneTransforms{};
	sceneTransforms.reserve(objects_count);

	uint32_t arrIdx = 0;
	for (const auto& [ent, matrix, draw] : view.each())
	{
		drawCommands.emplace_back(std::move(draw.Mesh->GetDrawCommand()));
		sceneTransforms.emplace_back(matrix.GetDecompose(), glm::vec4(draw.Mesh->GetMinPoint(), 1.0), glm::vec4(draw.Mesh->GetMaxPoint(), 1.0));

		matrix.AssignBuffer(GVkBuffer::ToView(_transformsPool, arrIdx * sizeof(SMeshData), sizeof(DecomposedMatrix)));
		draw._indirectCallView = GVkBuffer::ToView(_indirectPool, arrIdx * sizeof(VkDrawIndexedIndirectCommand), sizeof(VkDrawIndexedIndirectCommand));
		draw._conditionalView = GVkBuffer::ToView(_indirectPool, arrIdx * sizeof(VkDrawIndexedIndirectCommand) + offsetof(VkDrawIndexedIndirectCommand, instanceCount), sizeof(uint32_t));

		arrIdx++;
	}

	_transformsPoolView = GVkBuffer::ToView(_transformsPool, 0, t_size);
	_indirectPoolView = GVkBuffer::ToView(_indirectPool, 0, c_size);

	if (!drawCommands.empty() && !sceneTransforms.empty())
	{
		_updateCommands = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_TRANSFER_BIT, ECmdFlags::InFlight | ECmdFlags::Secondary);

		Scope->ExecuteForAllFrames([this, sceneTransforms, drawCommands] 
		{
			_updateCommands->UpdateBuffer(_transformsPoolView, (char*)sceneTransforms.data());
			_updateCommands->UpdateBuffer(_indirectPoolView, (char*)drawCommands.data());
		});
	}
	else
	{
		_updateCommands = VK_NULL_HANDLE;
	}

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

std::shared_ptr<GVkBufferView> IWorld::GetSceneTransformBuffer() const
{
	if (bDescriptorsDirty)
		const_cast<IWorld*>(this)->update_descriptors();

	return _transformsPoolView;
}

std::shared_ptr<GVkBufferView> IWorld::GetSceneIndirectBuffer() const
{
	if (bDescriptorsDirty)
		const_cast<IWorld*>(this)->update_descriptors();

	return _indirectPoolView;
}

std::shared_ptr<GVkCommandBuffer> IWorld::GetPreprocessCommands() const
{
	if (bDescriptorsDirty)
		const_cast<IWorld*>(this)->update_descriptors();

	if (_updateCommands == VK_NULL_HANDLE)
		return VK_NULL_HANDLE;

	// only update once
	if (_updateCommands->IsRecorded())
		_updateCommands = VK_NULL_HANDLE;
	else
		_updateCommands->Submit();

	return _updateCommands;
}