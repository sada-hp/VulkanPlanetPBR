#include "barrier.hpp"

VkPipelineStageFlags2 GVkBarrier::_evalStage(VkShaderStageFlags Shader) const
{
	switch (Shader)
	{
	case VK_SHADER_STAGE_COMPUTE_BIT:
		return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	case VK_SHADER_STAGE_VERTEX_BIT:
		return VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
	case VK_SHADER_STAGE_GEOMETRY_BIT:
		return VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;
	default:
		return VK_PIPELINE_STAGE_2_NONE;
	}
}

void GVkBarrier::GetBufferBarriers(std::vector<VkBufferMemoryBarrier2>& bufferBarriers, EOwnershipType ownershipType) const
{
	if (Buffer != VK_NULL_HANDLE)
	{
		if (ownershipType == EOwnershipType::Acquire && !ownershipReleasedFlags[Buffer->GetRoot()->_activeIndex()])
			return;

		if (ownershipType == EOwnershipType::Release)
			ownershipReleasedFlags[Buffer->GetRoot()->_activeIndex()] = true;

		if (ownershipType == EOwnershipType::Acquire)
			ownershipReleasedFlags[Buffer->GetRoot()->_activeIndex()] = false;

		VkBufferMemoryBarrier2 bufferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };

		bufferBarrier.dstQueueFamilyIndex = ownershipType == EOwnershipType::Ignored ? VK_QUEUE_FAMILY_IGNORED : dstQueue;
		bufferBarrier.srcQueueFamilyIndex = ownershipType == EOwnershipType::Ignored ? VK_QUEUE_FAMILY_IGNORED : srcQueue;

		bufferBarrier.dstAccessMask = barrierAccess;
		bufferBarrier.dstStageMask = barrierStage;

		bufferBarrier.buffer = Buffer->GetBuffer();
		bufferBarrier.offset = Buffer->GetOffset();
		bufferBarrier.size = Buffer->GetSize() == Buffer->GetRoot()->GetSize() - Buffer->GetOffset() ? VK_WHOLE_SIZE : Buffer->GetSize();

		// ? TODO
		bufferBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
		bufferBarrier.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		bufferBarriers.push_back(bufferBarrier);
	}
}

void GVkBarrier::GetImageBarriers(std::vector<VkImageMemoryBarrier2>& imageBarriers, EOwnershipType ownershipType) const
{
	if (Image != VK_NULL_HANDLE)
	{
		if (ownershipType == EOwnershipType::Acquire && !ownershipReleasedFlags[Image->GetRoot()->_activeIndex()])
			return;

		if (ownershipType == EOwnershipType::Release)
			ownershipReleasedFlags[Image->GetRoot()->_activeIndex()] = true;

		if (ownershipType == EOwnershipType::Acquire)
			ownershipReleasedFlags[Image->GetRoot()->_activeIndex()] = false;

		VkImageMemoryBarrier2 imageBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };

		imageBarrier.dstQueueFamilyIndex = ownershipType == EOwnershipType::Ignored ? VK_QUEUE_FAMILY_IGNORED : dstQueue;
		imageBarrier.srcQueueFamilyIndex = ownershipType == EOwnershipType::Ignored ? VK_QUEUE_FAMILY_IGNORED : srcQueue;

		imageBarrier.image = Image->GetImage();
		imageBarrier.newLayout = layout;

		imageBarrier.dstAccessMask = barrierAccess;
		imageBarrier.dstStageMask = barrierStage;

		std::vector<VkImageMemoryBarrier2> layerBarriers;
		for (uint32_t Layer = Image->BaseLayer(); Layer < Image->LastLayer(); Layer++)
		{
			std::vector<VkImageMemoryBarrier2> levelBarriers;
			for (uint32_t Level = Image->BaseLevel(); Level < Image->LastLevel(); Level++)
			{
				imageBarrier.srcAccessMask = Image->_getImageState(Level, Layer).access;
				imageBarrier.srcStageMask = Image->_getImageState(Level, Layer).stage;
				imageBarrier.oldLayout = Image->_getImageState(Level, Layer).layout;

				if (barrierAccess == VK_ACCESS_NONE)
					imageBarrier.dstAccessMask = imageBarrier.srcAccessMask;

				if (barrierStage == VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM)
					imageBarrier.dstStageMask = imageBarrier.srcStageMask;

				if (layout == VK_IMAGE_LAYOUT_UNDEFINED)
					imageBarrier.newLayout = imageBarrier.oldLayout;

				imageBarrier.subresourceRange.aspectMask = Image->GetSubresource().aspectMask;
				imageBarrier.subresourceRange.baseArrayLayer = Layer;
				imageBarrier.subresourceRange.baseMipLevel = Level;
				imageBarrier.subresourceRange.layerCount = 1;
				imageBarrier.subresourceRange.levelCount = 1;

				if (levelBarriers.empty()
					|| imageBarrier.srcAccessMask != levelBarriers.back().srcAccessMask
					|| imageBarrier.srcStageMask != levelBarriers.back().srcStageMask
					|| imageBarrier.oldLayout != levelBarriers.back().oldLayout
					|| imageBarrier.subresourceRange.baseMipLevel != levelBarriers.back().subresourceRange.baseMipLevel - 1
					)
				{
					levelBarriers.push_back(imageBarrier);
				}
				else
				{
					levelBarriers.back().subresourceRange.levelCount++;
				}
			}

			for (auto& levelBarrier : levelBarriers)
			{
				if (levelBarrier.subresourceRange.baseMipLevel + levelBarrier.subresourceRange.levelCount == Image->GetRoot()->GetMipLevelsCount())
					levelBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			}

			std::erase_if(levelBarriers, [&](VkImageMemoryBarrier2& levelBarrier) mutable
				{
					for (auto& layerBarrier : layerBarriers)
					{
						if (levelBarrier.oldLayout == layerBarrier.oldLayout
							&& levelBarrier.srcAccessMask == layerBarrier.srcAccessMask
							&& levelBarrier.srcStageMask == layerBarrier.srcStageMask
							&& levelBarrier.subresourceRange.levelCount == layerBarrier.subresourceRange.levelCount
							&& levelBarrier.subresourceRange.baseMipLevel == layerBarrier.subresourceRange.baseMipLevel)
						{
							layerBarrier.subresourceRange.layerCount++;
							return true;
						}
					}

					return false;
				});

			if (!levelBarriers.empty())
				layerBarriers.insert(layerBarriers.end(), levelBarriers.begin(), levelBarriers.end());
		}

		for (auto& layerBarrier : layerBarriers)
		{
			if (layerBarrier.subresourceRange.baseArrayLayer + layerBarrier.subresourceRange.layerCount == Image->GetRoot()->GetArrayLayers()
				|| layerBarrier.subresourceRange.baseArrayLayer + layerBarrier.subresourceRange.layerCount == Image->GetRoot()->GetExtent().depth)
			{
				layerBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			}
		}

		imageBarriers.insert(imageBarriers.end(), layerBarriers.begin(), layerBarriers.end());
		Image->_setImageState(layout, barrierStage, barrierAccess);
	}

	if (Swapchain != VK_NULL_HANDLE)
	{
		VkImageMemoryBarrier2 swapchainBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };

		swapchainBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		swapchainBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		swapchainBarrier.image = Swapchain->GetImage();
		swapchainBarrier.newLayout = layout;

		swapchainBarrier.dstAccessMask = barrierAccess;
		swapchainBarrier.dstStageMask = barrierStage;

		swapchainBarrier.srcAccessMask = Swapchain->GetImageState().access;
		swapchainBarrier.srcStageMask = Swapchain->GetImageState().stage;

		swapchainBarrier.oldLayout = Swapchain->GetImageState().layout;
		swapchainBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		swapchainBarrier.subresourceRange.baseArrayLayer = 0;
		swapchainBarrier.subresourceRange.layerCount = 1;

		swapchainBarrier.subresourceRange.baseMipLevel = 0;
		swapchainBarrier.subresourceRange.levelCount = 1;

		Swapchain->SetImageState(layout, barrierStage, barrierAccess);
		imageBarriers.push_back(swapchainBarrier);
	}
}

void GVkBarrier::Bind(VkCommandBuffer cmd, EOwnershipType ownershipType)
{
	std::vector<VkImageMemoryBarrier2> imageBarriers{};
	GetImageBarriers(imageBarriers, ownershipType);

	std::vector<VkBufferMemoryBarrier2> bufferBarriers{};
	GetBufferBarriers(bufferBarriers, ownershipType);

	VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencyInfo.bufferMemoryBarrierCount = bufferBarriers.size();
	dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();

	dependencyInfo.imageMemoryBarrierCount = imageBarriers.size();
	dependencyInfo.pImageMemoryBarriers = imageBarriers.data();

	vkCmdPipelineBarrier2(cmd, &dependencyInfo);
}