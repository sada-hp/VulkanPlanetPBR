#pragma once
#include "fence.hpp"
#include "scope.hpp"
#include "image.hpp"
#include "buffer.hpp"
#include "barrier.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include "framebuffer.hpp"
#include "render_pass.hpp"
#include "descriptor_set.hpp"

#include <concepts>

enum class ECmdFlags
{
	Default  = 0,
	InFlight = Bit(0)
};

DefineFlags(ECmdFlags)

class GVkCommandBuffer : public IVkObj
{
	struct _internalObj
	{
		VkCommandBuffer cmd = VK_NULL_HANDLE;
		std::shared_ptr<GVkFence> fence = VK_NULL_HANDLE;

		bool bRecording = false;
		uint32_t activeSubpass = 0;
		std::shared_ptr<GVkPipeline> pipeline = VK_NULL_HANDLE;
		std::shared_ptr<GVkRenderPass> renderPass = VK_NULL_HANDLE;
		std::shared_ptr<GVkFramebuffer> framebuffer = VK_NULL_HANDLE;

		std::vector<std::shared_ptr<IVkObj>> acquired_resources;
	};

private:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	std::vector<_internalObj> m_flightResources = {};
	VkQueueFlagBits m_qFlags;

private:
	size_t _activeIndex() const { return Scope->GetResourceIndex() % m_flightResources.size(); }
	const _internalObj& _activeObj() const { return m_flightResources.at(_activeIndex()); }
	_internalObj& _activeObj() { return m_flightResources.at(_activeIndex()); }

private:
	void _submitRecording(_internalObj& object);
	_internalObj& _updateRecording();
	void _nextSubpass();

public:
	GVkCommandBuffer(std::shared_ptr<RenderScope> Scope, VkQueueFlagBits Queue, ECmdFlags Flags = ECmdFlags::Default);
	~GVkCommandBuffer();

	void Submit();
	void BlitImage(std::shared_ptr<GVkImageView> Image, std::shared_ptr<GVkSwapchain> Swapchain);

	void UpdateBuffer(std::shared_ptr<GVkBufferView> View, char* Data);
	void CopyBuffer(std::shared_ptr<GVkBufferView> BufferSrc, std::shared_ptr<GVkBufferView> BufferDst);
	void CopyBuffer(std::shared_ptr<GVkBufferView> BufferSrc, std::shared_ptr<GVkImageView> ImageDst);

	void ClearImage(std::shared_ptr<GVkImageView> View, VkClearColorValue ClearValue);

	void BeginRenderPass(std::shared_ptr<GVkRenderPass> RenderPass, std::shared_ptr<GVkFramebuffer> Framebuffer);
	void EndRenderPass();

	void BindPipeline(std::shared_ptr<GVkPipeline> Pipeline);
	void BindDescriptorSet(uint32_t Slot, std::shared_ptr<GVkDescriptorSet> Set);

	void Draw(uint32_t VertexCount, uint32_t InstanceCount = 1u, uint32_t BaseVertex = 0u, uint32_t BaseInstance = 0u);

	void BindVertexBuffer(uint32_t Slot, std::shared_ptr<GVkBufferView> Buffer);
	void BindIndexBuffer(VkIndexType Type, std::shared_ptr<GVkBufferView> Buffer);
	void DrawIndexed(uint32_t Count, uint32_t InstanceCount = 1u, uint32_t BaseVertex = 0u, uint32_t FirstIndex = 0u, uint32_t BaseInstance = 0u);

public:
	template<std::derived_from<GVkBarrier> BarrierType, typename T, typename ...Args>
	inline void BindBarrier(std::shared_ptr<T> target, Args&&... args)
	{
		auto& object = _updateRecording();

		BarrierType Barrier(target, args...);
		Barrier.Bind(object.cmd);

		object.acquired_resources.emplace_back(target);
	}

	template<typename T>
	inline void PushConstants(uint32_t index, const T& data)
	{
		auto& object = _activeObj();

		assert(object.pipeline);

		const auto& constants = object.pipeline->_getPushConstants();
		if (constants.size() > index)
		{
			const auto& layoutData = constants[index];

			assert(sizeof(data) == layoutData.size);
			vkCmdPushConstants(object.cmd, object.pipeline->GetLayout(), layoutData.stageFlags, layoutData.offset, sizeof(T), (void*)&data);
		}
	}
};