#pragma once
#include "VulkanAPI/vertex.hpp"
#include "VulkanAPI/mesh.hpp"
#include "glm/glm.hpp"

struct GVkIndirectData
{
protected:
	std::shared_ptr<GVkBuffer> _indirectCall;
	std::shared_ptr<GVkBufferView> _indirectCallView;
	std::shared_ptr<GVkBufferView> _conditionalCountView;
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;

public:
	GVkIndirectData(std::shared_ptr<RenderScope> InScope, std::shared_ptr<IMesh> Mesh)
		: Scope(InScope)
	{
		if (Mesh->IndexCount() > 0)
		{
			VkDrawIndexedIndirectCommand command{};
			command.indexCount = Mesh->IndexCount();
			command.instanceCount = 1;
			_indirectCall = GVkBufferFactory::IndirectBufferFlight(Scope, sizeof(VkDrawIndexedIndirectCommand), &command);
		}
		else
		{
			VkDrawIndirectCommand command{};
			command.vertexCount = Mesh->VertexCount();
			command.instanceCount = 1;
			_indirectCall = GVkBufferFactory::IndirectBufferFlight(Scope, sizeof(VkDrawIndirectCommand), &command);
		}
	
		_indirectCallView = GVkBuffer::ToView(_indirectCall);
		_conditionalCountView = GVkBuffer::ToView(_indirectCall, offsetof(VkDrawIndirectCommand, instanceCount), sizeof(uint32_t));
	}

	std::shared_ptr<GVkBufferView> IndirectBuffer()
	{
		return _indirectCallView;
	}

	std::shared_ptr<GVkBufferView> ConditionalBuffer()
	{
		return _conditionalCountView;
	}
};