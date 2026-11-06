#pragma once
#include "swapchain.hpp"
#include "buffer.hpp"
#include "image.hpp"

enum class EOwnershipType
{
	Ignored,
	Acquire,
	Release
};

class GVkBarrier : public IVkObj
{
protected:
	std::shared_ptr<GVkSwapchain> Swapchain = VK_NULL_HANDLE;
	std::shared_ptr<GVkBufferView> Buffer = VK_NULL_HANDLE;
	std::shared_ptr<GVkImageView> Image = VK_NULL_HANDLE;

	uint32_t srcQueue = VK_QUEUE_FAMILY_IGNORED;
	uint32_t dstQueue = VK_QUEUE_FAMILY_IGNORED;

	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkPipelineStageFlags2 barrierStage = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
	VkAccessFlags2 barrierAccess = VK_ACCESS_NONE;
	mutable std::vector<bool> ownershipReleasedFlags;

protected:
	VkPipelineStageFlags2 _evalStage(VkShaderStageFlags Shader) const;

public:
	GVkBarrier(std::shared_ptr<GVkImageView> View, VkImageLayout Layout, VkPipelineStageFlags2 Stage, VkAccessFlags2 Access)
		: Image(View), layout(Layout), barrierStage(Stage), barrierAccess(Access)
	{
		ownershipReleasedFlags.resize(View->GetRoot()->FlightCount());
	}

	GVkBarrier(std::shared_ptr<GVkBufferView> View, VkPipelineStageFlags2 Stage, VkAccessFlags2 Access)
		: Buffer(View), barrierStage(Stage), barrierAccess(Access)
	{
		ownershipReleasedFlags.resize(View->GetRoot()->FlightCount());
	}

	GVkBarrier(std::shared_ptr<GVkSwapchain> View, VkImageLayout Layout, VkPipelineStageFlags2 Stage, VkAccessFlags2 Access)
		: Swapchain(View), layout(Layout), barrierStage(Stage), barrierAccess(Access)
	{
	}

	GVkBarrier(std::shared_ptr<GVkImageView> View, VkPipelineStageFlags2 Stage, VkAccessFlags2 Access, uint32_t Src, uint32_t Dst)
		: Image(View), barrierStage(Stage), barrierAccess(Access), srcQueue(Src), dstQueue(Dst)
	{
		ownershipReleasedFlags.resize(View->GetRoot()->FlightCount());
	}

	GVkBarrier(std::shared_ptr<GVkBufferView> View, VkPipelineStageFlags2 Stage, VkAccessFlags2 Access, uint32_t Src, uint32_t Dst)
		: Buffer(View), barrierStage(Stage), barrierAccess(Access), srcQueue(Src), dstQueue(Dst)
	{
		ownershipReleasedFlags.resize(View->GetRoot()->FlightCount());
	}

	void GetBufferBarriers(std::vector<VkBufferMemoryBarrier2>& bufferBarriers, EOwnershipType ownershipType = EOwnershipType::Ignored) const;
	void GetImageBarriers(std::vector<VkImageMemoryBarrier2>& imageBarriers, EOwnershipType ownershipType = EOwnershipType::Ignored) const;
	void Bind(VkCommandBuffer cmd, EOwnershipType ownershipType = EOwnershipType::Ignored);
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
			, View->GetRoot()->IsDepth() ? VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT  : VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
			, View->GetRoot()->IsDepth() ? VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT)
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
		: GVkBarrier(View, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE)
	{ }
};