#include "command_buffer.hpp"

GVkCommandBuffer::GVkCommandBuffer(std::shared_ptr<RenderScope> InScope, VkQueueFlagBits Queue, ECmdFlags Flags)
	: Scope(InScope), m_qFlags(Queue)
{
	if (CheckFlag(Flags, ECmdFlags::InFlight))
		m_flightResources.resize(Scope->GetMaxFramesInFlight());
	else
		m_flightResources.resize(1);

	VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (auto& object : m_flightResources)
	{
		Scope->GetQueue(m_qFlags).allocateCommandBuffer(&object.cmd);
		object.fence = std::make_shared<GVkFence>(Scope, EFenceFlags::CreateSignaled);
	}
}

GVkCommandBuffer::~GVkCommandBuffer()
{
	for (auto& object : m_flightResources)
	{
		if (object.bRecording)
			_submitRecording(object);

		object.fence->Wait();
		Scope->GetQueue(m_qFlags).freeCommandBuffer(object.cmd);
	}
}

GVkCommandBuffer::_internalObj& GVkCommandBuffer::_updateRecording()
{
	auto& object = _activeObj();

	if (!object.bRecording)
	{
		object.fence->Wait();

		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		vkBeginCommandBuffer(object.cmd, &beginInfo);
		
		object.acquired_resources.clear();
		object.bRecording = true;
	}

	return object;
}

void GVkCommandBuffer::_submitRecording(_internalObj& object)
{
	if (object.bRecording)
	{
		if (object.renderPass)
			EndRenderPass();

		vkEndCommandBuffer(object.cmd);

		object.fence->Reset();
		Scope->GetQueue(m_qFlags).submitCommandBuffer(object.cmd, object.fence->GetFence());

		object.pipeline = VK_NULL_HANDLE;
		object.bRecording = false;
	}
}

void GVkCommandBuffer::_nextSubpass()
{
	auto& object = _activeObj();

	if (object.renderPass)
	{
		vkCmdNextSubpass(object.cmd, VK_SUBPASS_CONTENTS_INLINE);
		object.activeSubpass++;
	}
}

void GVkCommandBuffer::Submit()
{
	auto& object = _activeObj();
	_submitRecording(object);
}

void GVkCommandBuffer::UpdateBuffer(std::shared_ptr<GVkBufferView> Buffer, char* Data)
{
	if (char* Mapped = (char*)Buffer->GetRoot()->Map())
	{
		memcpy(Mapped + Buffer->GetOffset(), Data, Buffer->GetSize());
		Buffer->GetRoot()->UnMap();
	}
	else
	{
		auto stageBuffer = std::make_shared<GVkBuffer>(Scope, Buffer->GetSize(), EBufferFlags::Mapped | EBufferFlags::TransferSrc);
		auto stageBufferView = GVkBuffer::ToView(stageBuffer);

		UpdateBuffer(stageBufferView, Data);
		CopyBuffer(stageBufferView, Buffer);
	}
}

void GVkCommandBuffer::CopyBuffer(std::shared_ptr<GVkBufferView> BufferSrc, std::shared_ptr<GVkBufferView> BufferDst)
{
	auto& object = _updateRecording();

	VkBufferCopy region{};
	region.dstOffset = BufferDst->GetOffset();
	region.srcOffset = BufferSrc->GetOffset();
	region.size = std::min(BufferSrc->GetSize(), BufferDst->GetSize());

	BindBarrier<GVkTransferDstBarrier>(BufferDst);
	BindBarrier<GVkTransferSrcBarrier>(BufferSrc);
	vkCmdCopyBuffer(object.cmd, BufferSrc->GetBuffer(), BufferDst->GetBuffer(), 1, &region);

	object.acquired_resources.emplace_back(BufferSrc);
	object.acquired_resources.emplace_back(BufferDst);
}

void GVkCommandBuffer::ClearImage(std::shared_ptr<GVkImageView> Image, VkClearColorValue ClearValue)
{
	auto& object = _updateRecording();

	BindBarrier<GVkTransferDstBarrier>(Image);
	vkCmdClearColorImage(object.cmd, Image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ClearValue, 1, &Image->GetSubresource());

	object.acquired_resources.emplace_back(Image);
}

void GVkCommandBuffer::BlitImage(std::shared_ptr<GVkImageView> Image, std::shared_ptr<GVkSwapchain> Swapchain)
{
	auto& object = _updateRecording();

	VkImageBlit imageBlit{};
	imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBlit.srcSubresource.baseArrayLayer = Image->GetSubresource().baseArrayLayer;
	imageBlit.srcSubresource.mipLevel = Image->GetSubresource().baseMipLevel;
	imageBlit.srcSubresource.layerCount = 1;

	imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBlit.dstSubresource.layerCount = 1;

	imageBlit.srcOffsets[1].x = Image->GetRoot()->GetExtent(Image->GetSubresource().baseMipLevel).width;
	imageBlit.srcOffsets[1].y = Image->GetRoot()->GetExtent(Image->GetSubresource().baseMipLevel).height;
	imageBlit.srcOffsets[1].z = 1;

	imageBlit.dstOffsets[1].x = Swapchain->GetExtent().width;
	imageBlit.dstOffsets[1].y = Swapchain->GetExtent().height;
	imageBlit.dstOffsets[1].z = 1;

	BindBarrier<GVkTransferSrcBarrier>(Image);
	BindBarrier<GVkTransferDstBarrier>(Swapchain);

	vkCmdBlitImage(object.cmd, Image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Swapchain->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);
	BindBarrier<GVkPresentBarrier>(Swapchain);

	object.acquired_resources.emplace_back(Swapchain);
	object.acquired_resources.emplace_back(Image);
}

void GVkCommandBuffer::BeginRenderPass(std::shared_ptr<GVkRenderPass> RenderPass, std::shared_ptr<GVkFramebuffer> Framebuffer)
{
	if (RenderPass && Framebuffer)
	{
		auto& object = _updateRecording();
		object.activeSubpass = 0;
		object.renderPass = RenderPass;
		object.framebuffer = Framebuffer;
		object.acquired_resources.emplace_back(RenderPass);
		object.acquired_resources.emplace_back(Framebuffer);

		uint32_t index = 0;
		auto& views = object.framebuffer->GetViews();
		for (auto& view : views)
		{
			auto attState = object.renderPass->_getAttachmentState(index);
			BindBarrier<GVkBarrier>(view, attState.initial_layout, attState.initial_stage, attState.initial_access);
			index++;
		}

		VkRenderPassBeginInfo beginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		beginInfo.clearValueCount = RenderPass->GetClearValues().size();
		beginInfo.pClearValues = RenderPass->GetClearValues().data();
		beginInfo.framebuffer = Framebuffer->GetFramebuffer();
		beginInfo.renderArea = Framebuffer->GetRenderArea();
		beginInfo.renderPass = RenderPass->GetRenderPass();
		vkCmdBeginRenderPass(object.cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkRect2D Scissor = Framebuffer->GetRenderArea();
		vkCmdSetScissor(object.cmd, 0, 1, &Scissor);

		VkViewport Viewport = { 0, 0, Scissor.extent.width, Scissor.extent.height, 0.0, 1.0 };
		vkCmdSetViewport(object.cmd, 0, 1, &Viewport);
	}
}

void GVkCommandBuffer::EndRenderPass()
{
	auto& object = _activeObj();

	if (object.renderPass != VK_NULL_HANDLE)
	{
		while (object.activeSubpass < (object.renderPass->GetSubpassCount() - 1))
			_nextSubpass();

		vkCmdEndRenderPass(object.cmd);

		uint32_t index = 0;
		auto& views = object.framebuffer->GetViews();

		for (auto& view : views)
		{
			auto attState = object.renderPass->_getAttachmentState(index);
			view->_setImageState(attState.final_layout, attState.final_stage, attState.final_access);
			BindBarrier<GVkBarrier>(view, attState.next_layout, attState.next_stage, attState.next_access);
			index++;
		}

		object.framebuffer = VK_NULL_HANDLE;
		object.renderPass = VK_NULL_HANDLE;
		object.pipeline = VK_NULL_HANDLE;
		object.activeSubpass = 0;
	}
}

void GVkCommandBuffer::BindPipeline(std::shared_ptr<GVkPipeline> Pipeline)
{
	if (Pipeline)
	{
		auto& object = _updateRecording();

		object.pipeline = Pipeline;

		if (object.renderPass)
		{
			assert(object.pipeline->GetSubpass() < object.renderPass->GetSubpassCount());

			while (object.activeSubpass < object.pipeline->GetSubpass())
				_nextSubpass();

			vkCmdBindPipeline(object.cmd, Pipeline->GetBindPoint(), Pipeline->GetPipeline());
			object.acquired_resources.emplace_back(Pipeline);
		}
	}
}

void GVkCommandBuffer::BindDescriptorSet(uint32_t Slot, std::shared_ptr<GVkDescriptorSet> Set)
{
	if (Set)
	{
		auto& object = _updateRecording();

		assert(object.pipeline);

		vkCmdBindDescriptorSets(object.cmd, object.pipeline->GetBindPoint(), object.pipeline->GetLayout(), Slot, 1, &Set->GetDescriptorSet(), 0, VK_NULL_HANDLE);
		object.acquired_resources.emplace_back(Set);
	}
}

void GVkCommandBuffer::Draw(uint32_t VertexCount, uint32_t InstanceCount, uint32_t BaseVertex, uint32_t BaseInstance)
{
	auto& object = _updateRecording();

	assert(object.renderPass && object.pipeline);
	vkCmdDraw(object.cmd, VertexCount, InstanceCount, BaseVertex, BaseInstance);
}

void GVkCommandBuffer::BindVertexBuffer(uint32_t Slot, std::shared_ptr<GVkBufferView> Buffer)
{
	auto& object = _updateRecording();

	VkBuffer vbo = Buffer->GetBuffer();
	VkDeviceSize offset = Buffer->GetOffset();
	vkCmdBindVertexBuffers(object.cmd, Slot, 1, &vbo, &offset);
	object.acquired_resources.emplace_back(Buffer);
}

void GVkCommandBuffer::BindIndexBuffer(VkIndexType Type, std::shared_ptr<GVkBufferView> Buffer)
{
	auto& object = _updateRecording();

	VkBuffer ibo = Buffer->GetBuffer();
	VkDeviceSize offset = Buffer->GetOffset();
	vkCmdBindIndexBuffer(object.cmd, ibo, offset, Type);
	object.acquired_resources.emplace_back(Buffer);
}

void GVkCommandBuffer::DrawIndexed(uint32_t Count, uint32_t InstanceCount, uint32_t BaseVertex, uint32_t FirstIndex, uint32_t BaseInstance)
{
	auto& object = _updateRecording();

	assert(object.renderPass && object.pipeline);
	vkCmdDrawIndexed(object.cmd, Count, InstanceCount, FirstIndex, BaseVertex, BaseInstance);
}