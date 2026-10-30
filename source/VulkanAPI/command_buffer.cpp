#include "command_buffer.hpp"

GVkCommandBuffer::GVkCommandBuffer(std::shared_ptr<RenderScope> InScope, VkQueueFlagBits Queue, ECmdFlags Flags)
	: Scope(InScope), m_qFlags(Queue), m_cmdFlags(Flags)
{
	if (CheckFlag(Flags, ECmdFlags::InFlight))
		m_flightResources.resize(Scope->GetMaxFramesInFlight());
	else
		m_flightResources.resize(1);

	VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (auto& object : m_flightResources)
	{
		if (CheckFlag(m_cmdFlags, ECmdFlags::Secondary))
			Scope->GetQueue(m_qFlags).allocateCommandBuffer2(&object.cmd);
		else
			Scope->GetQueue(m_qFlags).allocateCommandBuffer(&object.cmd);

		object.fence = std::make_shared<GVkFence>(Scope, EFenceFlags::CreateSignaled);
	}

	vkCmdBeginConditionalRendering = (PFN_vkCmdBeginConditionalRenderingEXT)vkGetInstanceProcAddr(Scope->GetInstance(), "vkCmdBeginConditionalRenderingEXT");
	vkCmdEndConditionalRendering = (PFN_vkCmdEndConditionalRenderingEXT)vkGetInstanceProcAddr(Scope->GetInstance(), "vkCmdEndConditionalRenderingEXT");
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

GVkCommandBuffer::_internalObj& GVkCommandBuffer::_updateRecording(const VkCommandBufferInheritanceInfo* inheritance)
{
	auto& object = _activeObj();

	if (!object.bRecording)
	{
		object.acquired_resources.clear();
		object.pipeline = VK_NULL_HANDLE;
		object.bRecorded  = false;
		object.bRecording = true;

		object.fence->Wait();

		if (CheckFlag(m_cmdFlags, ECmdFlags::Secondary))
		{
			VkCommandBufferInheritanceInfo inh{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };

			VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
			beginInfo.flags = inheritance ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0;
			beginInfo.pInheritanceInfo = inheritance ? inheritance : &inh;
			vkBeginCommandBuffer(object.cmd, &beginInfo);
		}
		else
		{
			VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(object.cmd, &beginInfo);
		}
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

		if (!CheckFlag(m_cmdFlags, ECmdFlags::Secondary))
		{
			object.fence->Reset();
			Scope->GetQueue(m_qFlags).submitCommandBuffer(object.cmd, object.fence->GetFence());
		}

		object.pipeline = VK_NULL_HANDLE;
		object.bRecording = false;
		object.bRecorded = true;
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

void GVkCommandBuffer::_acquire(std::shared_ptr<IVkObj> obj)
{
	auto& object = _activeObj();
	object.acquired_resources.emplace_back(std::move(obj));
}

VkCommandBuffer GVkCommandBuffer::GetCommandBuffer()
{
	auto& object = _activeObj();
	return object.cmd;
}

bool GVkCommandBuffer::IsRecorded()
{
	if (CheckFlag(m_cmdFlags, ECmdFlags::Secondary))
	{
		auto& object = _activeObj();
		return object.bRecorded;
	}
	return false;
}

void GVkCommandBuffer::Wait()
{
	for (auto& object : m_flightResources)
		object.fence->Wait();
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

	_acquire(BufferSrc);
	_acquire(BufferDst);
}

void GVkCommandBuffer::CopyBuffer(std::shared_ptr<GVkBufferView> BufferSrc, std::shared_ptr<GVkImageView> ImageDst)
{
	auto& object = _updateRecording();

	std::vector<VkBufferImageCopy> copies(ImageDst->GetLevelCount());
	for (uint32_t level = ImageDst->BaseLevel(); level < ImageDst->BaseLevel() + ImageDst->GetLevelCount(); level++)
	{
		copies[level].imageExtent = ImageDst->GetRoot()->GetExtent();
		copies[level].imageSubresource.aspectMask = ImageDst->GetSubresource().aspectMask;
		copies[level].imageSubresource.baseArrayLayer = ImageDst->BaseLayer();
		copies[level].imageSubresource.layerCount = ImageDst->GetLayerCount();
		copies[level].imageSubresource.mipLevel = level;
	}

	BindBarrier<GVkTransferSrcBarrier>(BufferSrc);
	BindBarrier<GVkTransferDstBarrier>(ImageDst);
	vkCmdCopyBufferToImage(object.cmd, BufferSrc->GetBuffer(), ImageDst->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copies.size(), copies.data());

	_acquire(BufferSrc);
	_acquire(ImageDst);
}

void GVkCommandBuffer::GenerateMipMaps(std::shared_ptr<GVkImage> Image)
{
	auto& object = _updateRecording();

	uint32_t Layers = Image->GetArrayLayers();
	uint32_t Levels = Image->GetMipLevelsCount();
	for (uint32_t layer = 0; layer < Layers; layer++)
	{
		for (uint32_t level = 1; level < Levels; level++)
		{
			auto prevview = GVkImage::ToView(Image, level - 1, layer, 1, 1);
			auto nextview = GVkImage::ToView(Image, level, layer, 1, 1);
			BlitImage(prevview, nextview);
		}
	}

	auto view = GVkImage::ToView(Image);
	BindBarrier<GVkShaderReadBarrier>(view, VK_SHADER_STAGE_FRAGMENT_BIT);
}

void GVkCommandBuffer::ClearBuffer(std::shared_ptr<GVkBufferView> View, uint32_t Value)
{
	auto& object = _updateRecording();

	BindBarrier<GVkTransferDstBarrier>(View);
	vkCmdFillBuffer(object.cmd, View->GetBuffer(), View->GetOffset(), View->GetSize(), Value);

	_acquire(View);
}

void GVkCommandBuffer::ClearImage(std::shared_ptr<GVkImageView> Image, VkClearColorValue ClearValue)
{
	auto& object = _updateRecording();

	BindBarrier<GVkTransferDstBarrier>(Image);
	vkCmdClearColorImage(object.cmd, Image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ClearValue, 1, &Image->GetSubresource());

	_acquire(Image);
}

void GVkCommandBuffer::ClearImage(std::shared_ptr<GVkImageView> View, VkClearDepthStencilValue ClearValue)
{
	auto& object = _updateRecording();

	BindBarrier<GVkTransferDstBarrier>(View);
	vkCmdClearDepthStencilImage(object.cmd, View->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ClearValue, 1, &View->GetSubresource());

	_acquire(View);
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

	_acquire(Swapchain);
	_acquire(Image);
}

void GVkCommandBuffer::BlitImage(std::shared_ptr<GVkImageView> Source, std::shared_ptr<GVkImageView> Destination)
{
	auto& object = _updateRecording();

	VkImageBlit imageBlit{};
	imageBlit.srcSubresource.aspectMask = Source->GetSubresource().aspectMask;
	imageBlit.srcSubresource.baseArrayLayer = Source->BaseLayer();
	imageBlit.srcSubresource.layerCount = Source->GetLayerCount();
	imageBlit.srcSubresource.mipLevel = Source->BaseLevel();

	imageBlit.srcOffsets[1].x = Source->GetRoot()->GetExtent(Source->BaseLevel()).width;
	imageBlit.srcOffsets[1].y = Source->GetRoot()->GetExtent(Source->BaseLevel()).height;
	imageBlit.srcOffsets[1].z = Source->GetRoot()->GetExtent(Source->BaseLevel()).depth;

	imageBlit.dstSubresource.aspectMask = Destination->GetSubresource().aspectMask;
	imageBlit.dstSubresource.baseArrayLayer = Destination->BaseLayer();
	imageBlit.dstSubresource.layerCount = Destination->GetLayerCount();
	imageBlit.dstSubresource.mipLevel = Destination->BaseLevel();

	imageBlit.dstOffsets[1].x = Destination->GetRoot()->GetExtent(Destination->BaseLevel()).width;
	imageBlit.dstOffsets[1].y = Destination->GetRoot()->GetExtent(Destination->BaseLevel()).height;
	imageBlit.dstOffsets[1].z = Destination->GetRoot()->GetExtent(Destination->BaseLevel()).depth;

	BindBarrier<GVkTransferSrcBarrier>(Source);
	BindBarrier<GVkTransferDstBarrier>(Destination);

	bool IsDepth = Source->GetRoot()->IsDepth();
	vkCmdBlitImage(object.cmd, Source->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Destination->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, IsDepth ? VK_FILTER_NEAREST : VK_FILTER_LINEAR);

	_acquire(Destination);
	_acquire(Source);
}

void GVkCommandBuffer::BeginRenderPass(std::shared_ptr<GVkRenderPass> RenderPass, std::shared_ptr<GVkFramebuffer> Framebuffer, VkSubpassContents Contents)
{
	if (RenderPass && Framebuffer)
	{
		bool secondary = CheckFlag(m_cmdFlags, ECmdFlags::Secondary);

		VkCommandBufferInheritanceInfo inheritanceInfo{};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.framebuffer = Framebuffer->GetFramebuffer();
		inheritanceInfo.renderPass = RenderPass->GetRenderPass();
		inheritanceInfo.subpass = 0;

		auto& object = _updateRecording(secondary ? &inheritanceInfo : VK_NULL_HANDLE);
		object.activeSubpass = 0;
		object.renderPass = RenderPass;
		object.framebuffer = Framebuffer;

		if (!secondary)
		{
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
			vkCmdBeginRenderPass(object.cmd, &beginInfo, Contents);
		}

		if (secondary || Contents != VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS)
		{
			VkRect2D Scissor = Framebuffer->GetRenderArea();
			vkCmdSetScissor(object.cmd, 0, 1, &Scissor);

			VkViewport Viewport = { 0, 0, Scissor.extent.width, Scissor.extent.height, 0.0, 1.0 };
			vkCmdSetViewport(object.cmd, 0, 1, &Viewport);
		}

		_acquire(RenderPass);
		_acquire(Framebuffer);
	}
}

void GVkCommandBuffer::SetRenderArea(const VkRect2D& Scissor, const VkViewport& Viewport)
{
	auto& object = _activeObj();
	
	if (object.renderPass)
	{
		vkCmdSetViewport(object.cmd, 0, 1, &Viewport);
		vkCmdSetScissor(object.cmd, 0, 1, &Scissor);
	}
}

void GVkCommandBuffer::NextSubpass()
{
	auto& object = _activeObj();

	assert(object.renderPass);
	_nextSubpass();
}

void GVkCommandBuffer::EndRenderPass()
{
	auto& object = _activeObj();

	if (object.renderPass != VK_NULL_HANDLE)
	{
		if (!CheckFlag(m_cmdFlags, ECmdFlags::Secondary))
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
		}

		object.framebuffer = VK_NULL_HANDLE;
		object.renderPass = VK_NULL_HANDLE;
		object.pipeline = VK_NULL_HANDLE;
		object.activeSubpass = 0;
	}
}

void GVkCommandBuffer::BindPipeline(std::shared_ptr<GVkPipeline> Pipeline)
{
	auto& object = _updateRecording();

	if (Pipeline && object.pipeline != Pipeline)
	{
		object.pipeline = Pipeline;

		if (object.renderPass)
		{
			assert(object.pipeline->GetSubpass() < object.renderPass->GetSubpassCount());

			while (object.activeSubpass < object.pipeline->GetSubpass())
				_nextSubpass();
		}

		vkCmdBindPipeline(object.cmd, Pipeline->GetBindPoint(), Pipeline->GetPipeline());
		_acquire(Pipeline);
	}
}

void GVkCommandBuffer::BindDescriptorSet(uint32_t Slot, std::shared_ptr<GVkDescriptorSet> Set)
{
	if (Set)
	{
		auto& object = _updateRecording();
		assert(object.pipeline);

		vkCmdBindDescriptorSets(object.cmd, object.pipeline->GetBindPoint(), object.pipeline->GetLayout(), Slot, 1, &Set->GetDescriptorSet(), 0, VK_NULL_HANDLE);
		_acquire(Set);
	}
}

void GVkCommandBuffer::Dispatch(uint32_t X, uint32_t Y, uint32_t Z)
{
	auto& object = _updateRecording();
	vkCmdDispatch(object.cmd, X, Y, Z);
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
	_acquire(Buffer);
}

void GVkCommandBuffer::BindIndexBuffer(VkIndexType Type, std::shared_ptr<GVkBufferView> Buffer)
{
	auto& object = _updateRecording();

	VkBuffer ibo = Buffer->GetBuffer();
	VkDeviceSize offset = Buffer->GetOffset();
	vkCmdBindIndexBuffer(object.cmd, ibo, offset, Type);
	_acquire(Buffer);
}

void GVkCommandBuffer::DrawIndexed(uint32_t Count, uint32_t InstanceCount, uint32_t BaseVertex, uint32_t FirstIndex, uint32_t BaseInstance)
{
	auto& object = _updateRecording();

	assert(object.renderPass && object.pipeline);
	vkCmdDrawIndexed(object.cmd, Count, InstanceCount, FirstIndex, BaseVertex, BaseInstance);
}

void GVkCommandBuffer::DrawIndirectCount(std::shared_ptr<GVkBuffer> IndirectBuffer, std::shared_ptr<GVkBuffer> CountBuffer, size_t CountOffset, uint32_t MaxCounts, uint32_t Stride)
{
	auto& object = _updateRecording();

	vkCmdDrawIndirectCount(object.cmd, IndirectBuffer->GetBuffer(), 0, CountBuffer->GetBuffer(), CountOffset, MaxCounts, Stride);
	_acquire(IndirectBuffer);
	_acquire(CountBuffer);
}

void GVkCommandBuffer::DrawIndirectIndexedCount(std::shared_ptr<GVkBuffer> IndirectBuffer, std::shared_ptr<GVkBuffer> CountBuffer, size_t CountOffset, uint32_t MaxCounts, uint32_t Stride)
{
	auto& object = _updateRecording();

	vkCmdDrawIndexedIndirectCount(object.cmd, IndirectBuffer->GetBuffer(), 0, CountBuffer->GetBuffer(), CountOffset, MaxCounts, Stride);
	_acquire(IndirectBuffer);
	_acquire(CountBuffer);
}

void GVkCommandBuffer::DrawIndirect(std::shared_ptr<GVkBufferView> IndirectBuffer)
{
	auto& object = _updateRecording();

	vkCmdDrawIndirect(object.cmd, IndirectBuffer->GetBuffer(), IndirectBuffer->GetOffset(), IndirectBuffer->GetSize() / sizeof(VkDrawIndirectCommand), sizeof(VkDrawIndirectCommand));
	_acquire(IndirectBuffer);
}

void GVkCommandBuffer::DrawIndirectIndexed(std::shared_ptr<GVkBufferView> IndirectBuffer)
{
	auto& object = _updateRecording();

	vkCmdDrawIndexedIndirect(object.cmd, IndirectBuffer->GetBuffer(), IndirectBuffer->GetOffset(), IndirectBuffer->GetSize() / sizeof(VkDrawIndexedIndirectCommand), sizeof(VkDrawIndexedIndirectCommand));
	_acquire(IndirectBuffer);
}

void GVkCommandBuffer::BeginConditionalRendering(std::shared_ptr<GVkBufferView> Buffer)
{
	if (vkCmdBeginConditionalRendering)
	{
		auto& object = _updateRecording();

		VkConditionalRenderingBeginInfoEXT beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
		beginInfo.buffer = Buffer->GetBuffer();
		beginInfo.offset = Buffer->GetOffset();
		beginInfo.flags = 0;

		vkCmdBeginConditionalRendering(object.cmd, &beginInfo);
		_acquire(Buffer);
	}
}

void GVkCommandBuffer::EndConditionalRendering()
{
	if (vkCmdEndConditionalRendering)
	{
		auto& object = _updateRecording();
		vkCmdEndConditionalRendering(object.cmd);
	}
}

void GVkCommandBuffer::Execute(std::shared_ptr<GVkCommandBuffer> Commands)
{
	auto& object = _updateRecording();

	VkCommandBuffer other = Commands->GetCommandBuffer();
	vkCmdExecuteCommands(object.cmd, 1, &other);
	_acquire(Commands);
}

void GVkCommandBuffer::BindBarriers(const std::vector<VkBufferMemoryBarrier2>& buffers, const std::vector<VkImageMemoryBarrier2>& images)
{
	VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencyInfo.bufferMemoryBarrierCount = buffers.size();
	dependencyInfo.pBufferMemoryBarriers = buffers.data();

	dependencyInfo.imageMemoryBarrierCount = images.size();
	dependencyInfo.pImageMemoryBarriers = images.data();

	auto& object = _updateRecording();
	vkCmdPipelineBarrier2(object.cmd, &dependencyInfo);
}