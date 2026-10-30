#include "world.hpp"

IWorld::IWorld(std::shared_ptr<RenderScope> InScope)
	: Scope(InScope)
{
	registry.on_construct<GDrawable>().connect<&IWorld::register_gpu_actor>(this);
	registry.on_destroy<GDrawable>().connect<&IWorld::remove_gpu_actor>(this);
	registry.on_update<GDrawable>().connect<&IWorld::update_gpu_actor>(this);

	_indirectPool = GVkBufferFactory::Buffer(Scope, EBufferFlags::Storage | EBufferFlags::Conditional | EBufferFlags::Indirect | EBufferFlags::InFlight, sizeof(VkDrawIndirectCommand) * 16192);
	_transformsPool = GVkBufferFactory::Buffer(Scope, EBufferFlags::Storage | EBufferFlags::Host | EBufferFlags::Vertex, sizeof(glm::mat4) * 16192);
}

IWorld::~IWorld()
{
}

void IWorld::register_gpu_actor(entt::registry& reg, entt::entity ent)
{
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
	size_t objects_count = view.size_hint();

	size_t t_size = sizeof(glm::mat4) * objects_count;
	if (t_size > _transformsPool->GetSize())
	{
		_transformsPool = GVkBufferFactory::Buffer(Scope, EBufferFlags::Storage | EBufferFlags::Host | EBufferFlags::Vertex, t_size);
	}

	size_t c_size = sizeof(VkDrawIndexedIndirectCommand) * objects_count;
	if (c_size > _indirectPool->GetSize())
	{
		_indirectPool = GVkBufferFactory::Buffer(Scope, EBufferFlags::Storage | EBufferFlags::Conditional | EBufferFlags::Indirect | EBufferFlags::InFlight, c_size);
	}

	std::vector<VkDrawIndexedIndirectCommand> drawCommands{};
	drawCommands.reserve(objects_count);

	std::vector<glm::mat4> sceneTransforms{};
	sceneTransforms.reserve(objects_count);

	uint32_t arrIdx = 0;
	for (const auto& [ent, matrix, draw] : view.each())
	{
		drawCommands.emplace_back(std::move(draw.Mesh->GetDrawCommand()));
		sceneTransforms.emplace_back(matrix.GetMatrix());

		matrix.AssignBuffer(GVkBuffer::ToView(_transformsPool, arrIdx * sizeof(glm::mat4), sizeof(glm::mat4)));
		draw._indirectCallView = GVkBuffer::ToView(_indirectPool, arrIdx * sizeof(VkDrawIndexedIndirectCommand), sizeof(VkDrawIndexedIndirectCommand));
		draw._conditionalView = GVkBuffer::ToView(_indirectPool, arrIdx * sizeof(VkDrawIndexedIndirectCommand) + offsetof(VkDrawIndexedIndirectCommand, instanceCount), sizeof(uint32_t));

		arrIdx++;
	}

	{
		auto updateCommands = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_GRAPHICS_BIT);
		updateCommands->UpdateBuffer(GVkBuffer::ToView(_transformsPool, 0, t_size), (char*)sceneTransforms.data());

		Scope->ExecuteForAllFrames([this, updateCommands, c_size, drawCommands]() {
			updateCommands->UpdateBuffer(GVkBuffer::ToView(_indirectPool, 0, c_size), (char*)drawCommands.data());
		});
	}

	DescriptorSetDescriptor DSDescIndirect;
	DSDescIndirect.AddStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT, GVkBuffer::ToView(_indirectPool, 0, c_size));
	_indirectSet = DSDescIndirect.Allocate(Scope);

	DescriptorSetDescriptor DSDescTransform;
	DSDescTransform.AddStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT, GVkBuffer::ToView(_transformsPool, 0, t_size));
	_transformsSet = DSDescTransform.Allocate(Scope);

	_syncCommandBuffer = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_GRAPHICS_BIT, ECmdFlags::InFlight | ECmdFlags::Secondary);
	_drawCommandBuffer = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_GRAPHICS_BIT, ECmdFlags::InFlight | ECmdFlags::Secondary);

	bDescriptorsDirty = false;
}

std::shared_ptr<GVkCommandBuffer> IWorld::GetPrerecordedSyncCommands() const
{
	if (!_syncCommandBuffer)
		return VK_NULL_HANDLE;

	if (!_syncCommandBuffer->IsRecorded())
	{
		auto view = registry.view<EllipsoidMatrix, GDrawable>();

		_syncCommandBuffer->BindBarrier<GVkBarrier>(GVkBuffer::ToView(_indirectPool, 0, view.size_hint() * sizeof(VkDrawIndexedIndirectCommand)), VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT | VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT);
		_syncCommandBuffer->Submit();
	}

	return _syncCommandBuffer;
}

std::shared_ptr<GVkCommandBuffer> IWorld::GetPrerecordedDrawCommands(std::shared_ptr<GVkRenderPass> RenderPass, std::shared_ptr<GVkFramebuffer> Framebuffer, std::shared_ptr<GVkDescriptorSet> StaticSet) const
{
	if (!_drawCommandBuffer)
		return VK_NULL_HANDLE;

	if (!_drawCommandBuffer->IsRecorded())
	{
		_drawCommandBuffer->BeginRenderPass(RenderPass, Framebuffer);

		auto view = registry.view<EllipsoidMatrix, GDrawable>();
		for (const auto& [ent, matrix, draw] : view.each())
		{
			_drawCommandBuffer->BeginConditionalRendering(draw._conditionalView);
			{
				_drawCommandBuffer->BindVertexBuffer(0, draw.Mesh->VertexBuffer());
				_drawCommandBuffer->BindVertexBuffer(1, matrix.GetBufferGPU());

				bool IsIndexed = draw.Mesh->IndexCount() > 0;

				if (IsIndexed)
					_drawCommandBuffer->BindIndexBuffer(draw.Mesh->IndexType(), draw.Mesh->IndexBuffer());

				for (auto& material : draw.Materials)
				{
					_drawCommandBuffer->BindPipeline(material->GetPipeline());
					_drawCommandBuffer->BindDescriptorSet(0, StaticSet);

					if (auto descriptorset = material->GetDescriptorSet())
						_drawCommandBuffer->BindDescriptorSet(1, descriptorset);

					if (IsIndexed)
						_drawCommandBuffer->DrawIndirectIndexed(draw._indirectCallView);
					else
						_drawCommandBuffer->DrawIndirect(draw._indirectCallView);
				}
			}
			_drawCommandBuffer->EndConditionalRendering();
		}

		_drawCommandBuffer->Submit();
	}

	return _drawCommandBuffer;
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

