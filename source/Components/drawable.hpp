#pragma once
#include "Math/PlanetoidMatrix.hpp"
#include "Materials/IMaterial.hpp"
#include "VulkanAPI/indirect.hpp"
#include "VulkanAPI/mesh.hpp"
#include "IComponent.hpp"

struct GDrawable : public IComponent
{
	friend class GMeshPreprocessStage;
	friend class GCullingStage;
	friend class GMeshStage;

private:
	std::shared_ptr<GVkCommandBuffer> _commandBuffer = VK_NULL_HANDLE;
	std::shared_ptr<GVkIndirectData> _drawCall = VK_NULL_HANDLE;

public:
	std::shared_ptr<IMesh> Mesh = nullptr;
	std::vector<std::shared_ptr<IMaterial>> Materials = {};

private:
	void init_static_calls()
	{
		_drawCall = std::make_shared<GVkIndirectData>(Mesh->GetScope(), Mesh);
		_commandBuffer = std::make_shared<GVkCommandBuffer>(Mesh->GetScope(), VK_QUEUE_GRAPHICS_BIT, ECmdFlags::InFlight | ECmdFlags::Secondary);
	}

private:
	VkCommandBuffer _getPrerecordedCommands(std::shared_ptr<GVkRenderPass> RenderPass, std::shared_ptr<GVkFramebuffer> Framebuffer, std::shared_ptr<GVkBufferView> TransformView, std::shared_ptr<GVkDescriptorSet> StaticSet) const
	{
		if (!_commandBuffer->IsRecorded())
		{
			_commandBuffer->BeginRenderPass(RenderPass, Framebuffer);

			_commandBuffer->BeginConditionalRendering(_drawCall->ConditionalBuffer());
			{
				_commandBuffer->BindVertexBuffer(0, Mesh->VertexBuffer());
				_commandBuffer->BindVertexBuffer(1, TransformView);

				bool IsIndexed = Mesh->IndexCount() > 0;

				if (IsIndexed)
					_commandBuffer->BindIndexBuffer(Mesh->IndexType(), Mesh->IndexBuffer());

				for (auto& material : Materials)
				{
					_commandBuffer->BindPipeline(material->GetPipeline());
					_commandBuffer->BindDescriptorSet(0, StaticSet);

					if (auto descriptorset = material->GetDescriptorSet())
						_commandBuffer->BindDescriptorSet(1, descriptorset);

					if (IsIndexed)
						_commandBuffer->DrawIndirectIndexed(_drawCall->IndirectBuffer());
					else
						_commandBuffer->DrawIndirect(_drawCall->IndirectBuffer());
				}
			}
			_commandBuffer->EndConditionalRendering();
			_commandBuffer->Submit();
		}

		return _commandBuffer->GetCommandBuffer();
	}

public:
	GDrawable(std::shared_ptr<IMesh> InMesh, std::shared_ptr<IMaterial> InMaterial)
		: Materials({ InMaterial })
		, Mesh(InMesh)
	{
		init_static_calls();
	}

	GDrawable(std::shared_ptr<IMesh> InMesh, std::vector<std::shared_ptr<IMaterial>> InMaterials)
		: Materials(InMaterials)
		, Mesh(InMesh)
	{
		init_static_calls();
	}

	GDrawable(const GDrawable& other)
		: Materials(other.Materials)
		, Mesh(other.Mesh)
	{
		init_static_calls();
	}

	virtual ~GDrawable()
	{
	}

	std::shared_ptr<GVkIndirectData> GetIndirectData()
	{
		return _drawCall;
	}
};