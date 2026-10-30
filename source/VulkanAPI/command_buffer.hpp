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

#include <unordered_set>
#include <concepts>

enum class ECmdFlags
{
	Default  = 0,
	InFlight  = Bit(0),
	Secondary = Bit(1)
};

DefineFlags(ECmdFlags)

class GVkCommandBuffer : public IVkObj
{
	struct _internalObj
	{
		VkCommandBuffer cmd = VK_NULL_HANDLE;
		std::shared_ptr<GVkFence> fence = VK_NULL_HANDLE;

		bool bRecorded = false;
		bool bRecording = false;
		uint32_t activeSubpass = 0;
		std::shared_ptr<GVkPipeline> pipeline = VK_NULL_HANDLE;
		std::shared_ptr<GVkRenderPass> renderPass = VK_NULL_HANDLE;
		std::shared_ptr<GVkFramebuffer> framebuffer = VK_NULL_HANDLE;
		std::vector<std::shared_ptr<IVkObj>> acquired_resources;
	};

private:
	PFN_vkCmdBeginConditionalRenderingEXT vkCmdBeginConditionalRendering;
	PFN_vkCmdEndConditionalRenderingEXT vkCmdEndConditionalRendering;

private:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	std::vector<_internalObj> m_flightResources = {};

	ECmdFlags m_cmdFlags = ECmdFlags::Default;
	VkQueueFlagBits m_qFlags;

private:
	size_t _activeIndex() const { return Scope->GetResourceIndex() % m_flightResources.size(); }
	const _internalObj& _activeObj() const { return m_flightResources[_activeIndex()]; }
	_internalObj& _activeObj() { return m_flightResources[_activeIndex()]; }

private:
	void _submitRecording(_internalObj& object);
	void _acquire(std::shared_ptr<IVkObj> obj);
	
	_internalObj& _updateRecording(const VkCommandBufferInheritanceInfo* inheritance = VK_NULL_HANDLE);
	void _nextSubpass();

public:
	GVkCommandBuffer(std::shared_ptr<RenderScope> Scope, VkQueueFlagBits Queue, ECmdFlags Flags = ECmdFlags::Default);
	~GVkCommandBuffer();

	VkCommandBuffer GetCommandBuffer();

	bool IsRecorded();

	void Wait();
	void Submit();
	void BlitImage(std::shared_ptr<GVkImageView> Image, std::shared_ptr<GVkSwapchain> Swapchain);
	void BlitImage(std::shared_ptr<GVkImageView> Source, std::shared_ptr<GVkImageView> Destination);

	void UpdateBuffer(std::shared_ptr<GVkBufferView> View, char* Data);
	void CopyBuffer(std::shared_ptr<GVkBufferView> BufferSrc, std::shared_ptr<GVkBufferView> BufferDst);
	void CopyBuffer(std::shared_ptr<GVkBufferView> BufferSrc, std::shared_ptr<GVkImageView> ImageDst);

	void GenerateMipMaps(std::shared_ptr<GVkImage> Image);

	void ClearBuffer(std::shared_ptr<GVkBufferView> View, uint32_t Value);
	void ClearImage(std::shared_ptr<GVkImageView> View, VkClearColorValue ClearValue);
	void ClearImage(std::shared_ptr<GVkImageView> View, VkClearDepthStencilValue ClearValue);

	void BeginRenderPass(std::shared_ptr<GVkRenderPass> RenderPass, std::shared_ptr<GVkFramebuffer> Framebuffer, VkSubpassContents Contents = VK_SUBPASS_CONTENTS_INLINE);
	void EndRenderPass();

	void BindPipeline(std::shared_ptr<GVkPipeline> Pipeline);
	void BindDescriptorSet(uint32_t Slot, std::shared_ptr<GVkDescriptorSet> Set);

	void Draw(uint32_t VertexCount, uint32_t InstanceCount = 1u, uint32_t BaseVertex = 0u, uint32_t BaseInstance = 0u);
	void Dispatch(uint32_t X, uint32_t Y, uint32_t Z);

	void BindVertexBuffer(uint32_t Slot, std::shared_ptr<GVkBufferView> Buffer);
	void BindIndexBuffer(VkIndexType Type, std::shared_ptr<GVkBufferView> Buffer);
	void DrawIndexed(uint32_t Count, uint32_t InstanceCount = 1u, uint32_t BaseVertex = 0u, uint32_t FirstIndex = 0u, uint32_t BaseInstance = 0u);

	void DrawIndirectCount(std::shared_ptr<GVkBuffer> IndirectBuffer, std::shared_ptr<GVkBuffer> CountBuffer, size_t CountOffset, uint32_t MaxCounts = 1, uint32_t Stride = sizeof(VkDrawIndirectCommand));
	void DrawIndirectIndexedCount(std::shared_ptr<GVkBuffer> IndirectBuffer, std::shared_ptr<GVkBuffer> CountBuffer, size_t CountOffset, uint32_t MaxCounts = 1, uint32_t Stride = sizeof(VkDrawIndexedIndirectCommand));
	
	void DrawIndirect(std::shared_ptr<GVkBufferView> IndirectBuffer);
	void DrawIndirectIndexed(std::shared_ptr<GVkBufferView> IndirectBuffer);

	void BeginConditionalRendering(std::shared_ptr<GVkBufferView> Buffer);
	void EndConditionalRendering();

	void SetRenderArea(const VkRect2D& Scissor, const VkViewport& Viewport);
	void NextSubpass();

	void Execute(const std::vector<VkCommandBuffer>& Commands);

public:
	template<std::derived_from<GVkBarrier> BarrierType, typename T, typename ...Args>
	inline void BindBarrier(std::shared_ptr<T> target, Args&&... args)
	{
		auto& object = _updateRecording();

		BarrierType Barrier(target, args...);
		Barrier.Bind(object.cmd);

		_acquire(target);
	}

	void BindBarriers(const std::vector<VkBufferMemoryBarrier2>& buffers, const std::vector<VkImageMemoryBarrier2>& images);

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