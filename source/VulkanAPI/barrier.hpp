#pragma once
#include "swapchain.hpp"
#include "buffer.hpp"
#include "image.hpp"

class GVkBarrier : public IVkObj
{
protected:
	std::shared_ptr<GVkSwapchain> Swapchain = VK_NULL_HANDLE;
	std::shared_ptr<GVkBufferView> Buffer = VK_NULL_HANDLE;
	std::shared_ptr<GVkImageView> Image = VK_NULL_HANDLE;

	VkQueueFlags srcQueue = VK_QUEUE_FAMILY_IGNORED;
	VkQueueFlags dstQueue = VK_QUEUE_FAMILY_IGNORED;

	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkPipelineStageFlags2 barrierStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkAccessFlags2 barrierAccess = VK_ACCESS_NONE;

protected:
	VkPipelineStageFlags2 _evalStage(VkShaderStageFlags Shader)
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

public:
	GVkBarrier(std::shared_ptr<GVkImageView> View, VkImageLayout Layout, VkPipelineStageFlags2 Stage, VkAccessFlags2 Access)
		: Image(View), layout(Layout), barrierStage(Stage), barrierAccess(Access)
	{
	}

	GVkBarrier(std::shared_ptr<GVkBufferView> View, VkPipelineStageFlags2 Stage, VkAccessFlags2 Access)
		: Buffer(View), barrierStage(Stage), barrierAccess(Access)
	{
	}

	GVkBarrier(std::shared_ptr<GVkSwapchain> View, VkImageLayout Layout, VkPipelineStageFlags2 Stage, VkAccessFlags2 Access)
		: Swapchain(View), layout(Layout), barrierStage(Stage), barrierAccess(Access)
	{
	}

	void Bind(VkCommandBuffer cmd);
};

class GVkTransferSrcBarrier : public GVkBarrier
{
public:
	GVkTransferSrcBarrier(std::shared_ptr<GVkImageView> View)
		: GVkBarrier(View, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT)
	{ }

	GVkTransferSrcBarrier(std::shared_ptr<GVkBufferView> View)
		: GVkBarrier(View, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT)
	{ }
};

class GVkTransferDstBarrier : public GVkBarrier
{
public:
	GVkTransferDstBarrier(std::shared_ptr<GVkImageView> View)
		: GVkBarrier(View, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT)
	{ }

	GVkTransferDstBarrier(std::shared_ptr<GVkSwapchain> View)
		: GVkBarrier(View, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT)
	{ }

	GVkTransferDstBarrier(std::shared_ptr<GVkBufferView> View)
		: GVkBarrier(View, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT)
	{ }
};

class GVkRenderTargetBarrier : public GVkBarrier
{
public:
	GVkRenderTargetBarrier(std::shared_ptr<GVkImageView> View)
		: GVkBarrier(View
			, View->GetRoot()->IsDepth() ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			, View->GetRoot()->IsDepth() ? VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT  : VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
			, View->GetRoot()->IsDepth() ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)
	{ }
};

class GVkShaderReadBarrier : public GVkBarrier
{
public:
	GVkShaderReadBarrier(std::shared_ptr<GVkImageView> View, VkShaderStageFlags Shader)
		: GVkBarrier(View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _evalStage(Shader), VK_ACCESS_2_SHADER_READ_BIT)
	{ }

	GVkShaderReadBarrier(std::shared_ptr<GVkBufferView> View, VkShaderStageFlags Shader)
		: GVkBarrier(View, _evalStage(Shader), VK_ACCESS_2_SHADER_READ_BIT)
	{ }
};

class GVkShaderStorageBarrier : public GVkBarrier
{
public:
	GVkShaderStorageBarrier(std::shared_ptr<GVkImageView> View, VkShaderStageFlags Shader)
		: GVkBarrier(View, VK_IMAGE_LAYOUT_GENERAL, _evalStage(Shader), VK_ACCESS_2_SHADER_WRITE_BIT)
	{ }

	GVkShaderStorageBarrier(std::shared_ptr<GVkBufferView> View, VkShaderStageFlags Shader)
		: GVkBarrier(View, _evalStage(Shader), VK_ACCESS_2_SHADER_WRITE_BIT)
	{ }
};

class GVkPresentBarrier : public GVkBarrier
{
public:
	GVkPresentBarrier(std::shared_ptr<GVkSwapchain> View)
		: GVkBarrier(View, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT)
	{ }
};