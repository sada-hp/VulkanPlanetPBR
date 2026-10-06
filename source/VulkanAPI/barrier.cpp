#include "barrier.hpp"

void GVkBarrier::Bind(VkCommandBuffer cmd)
{
	std::vector<VkBufferMemoryBarrier2> bufferBarriers;
	std::vector<VkImageMemoryBarrier2> imageBarriers;

	if (Image != VK_NULL_HANDLE)
	{
		VkImageMemoryBarrier2 imageBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };

		imageBarrier.dstQueueFamilyIndex = dstQueue;
		imageBarrier.srcQueueFamilyIndex = srcQueue;

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
				imageBarrier.srcAccessMask = Image->GetRoot()->_getImageState(Level, Layer).access;
				imageBarrier.srcStageMask = Image->GetRoot()->_getImageState(Level, Layer).stage;
				imageBarrier.oldLayout = Image->GetRoot()->_getImageState(Level, Layer).layout;
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

		swapchainBarrier.dstQueueFamilyIndex = dstQueue;
		swapchainBarrier.srcQueueFamilyIndex = srcQueue;

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

	if (Buffer != VK_NULL_HANDLE)
	{
		VkBufferMemoryBarrier2 bufferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };

		bufferBarrier.dstQueueFamilyIndex = dstQueue;
		bufferBarrier.srcQueueFamilyIndex = srcQueue;

		bufferBarrier.dstAccessMask = barrierAccess;
		bufferBarrier.dstStageMask = barrierStage;

		bufferBarrier.buffer = Buffer->GetBuffer();
		bufferBarrier.offset = Buffer->GetOffset();
		bufferBarrier.size = Buffer->GetSize();

		// ? TODO
		bufferBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
		bufferBarrier.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		bufferBarriers.push_back(bufferBarrier);
	}

	VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencyInfo.bufferMemoryBarrierCount = bufferBarriers.size();
	dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();

	dependencyInfo.imageMemoryBarrierCount = imageBarriers.size();
	dependencyInfo.pImageMemoryBarriers = imageBarriers.data();

	vkCmdPipelineBarrier2(cmd, &dependencyInfo);
}