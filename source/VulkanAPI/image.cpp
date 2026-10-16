#include "pch.hpp"
#include "image.hpp"

GVkImage::GVkImage(std::shared_ptr<RenderScope> InScope, VkFormat Format, VkExtent3D Extents, EImageFlags Flags)
	: Scope(InScope), m_Format(Format), m_Extents(Extents)
{
	VmaAllocationCreateInfo allocCreateInfo{};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	VkImageCreateInfo imageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	if (CheckFlag(Flags, EImageFlags::DepthAsLayers))
	{
		imageCreateInfo.arrayLayers = Extents.depth;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent = { Extents.width, Extents.height, 1 };
	}
	else
	{
		if (Extents.depth > 1)
		{
			imageCreateInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
		}
		else
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		
		imageCreateInfo.extent = Extents;
		imageCreateInfo.arrayLayers = 1;
	}

	if (CheckFlag(Flags, EImageFlags::AllocateMipMaps))
	{
		imageCreateInfo.mipLevels = std::max(std::max(std::log2(Extents.width), std::log2(Extents.height)), 1.0);
	}
	else
	{
		imageCreateInfo.mipLevels = 1;
	}

	if (CheckFlag(Flags, EImageFlags::Cubemap))
	{
		imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

	if (CheckFlag(Flags, EImageFlags::Storage))
		imageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

	if (CheckFlag(Flags, EImageFlags::TransferTarget))
		imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if (CheckFlag(Flags, EImageFlags::TransferSource))
		imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if (CheckFlag(Flags, EImageFlags::SubpassInput))
		imageCreateInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

	bool IsDepthFormat = Format == VK_FORMAT_D16_UNORM || Format == VK_FORMAT_D16_UNORM_S8_UINT || Format == VK_FORMAT_D24_UNORM_S8_UINT 
		|| Format == VK_FORMAT_D32_SFLOAT_S8_UINT || Format == VK_FORMAT_D32_SFLOAT;

	if (CheckFlag(Flags, EImageFlags::RenderTarget))
	{
		if (IsDepthFormat)
			imageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		else
			imageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.format = Format;

	m_SubRange.aspectMask = IsDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	m_SubRange.layerCount = imageCreateInfo.arrayLayers;
	m_SubRange.levelCount = imageCreateInfo.mipLevels;
	m_Type = imageCreateInfo.imageType;

	if (CheckFlag(Flags, EImageFlags::InFlight))
		m_flightResources.resize(Scope->GetMaxFramesInFlight());
	else
		m_flightResources.resize(1);

	VkImageFormatProperties supportedProperties{};
	assert(vkGetPhysicalDeviceImageFormatProperties(Scope->GetPhysicalDevice(), Format, imageCreateInfo.imageType, imageCreateInfo.tiling, imageCreateInfo.usage, imageCreateInfo.flags, &supportedProperties) == VK_SUCCESS);

	for (auto& object : m_flightResources)
	{
		auto res = vmaCreateImage(Scope->GetAllocator(), &imageCreateInfo, &allocCreateInfo, &object.image, &object.memory, VK_NULL_HANDLE);
		assert(res == VK_SUCCESS);
		object.layouts = std::vector{ std::max(imageCreateInfo.arrayLayers, Extents.depth), std::vector(m_SubRange.levelCount, GVkImageState{}) };
	}
}

GVkImage::~GVkImage()
{
	for (auto& object : m_flightResources)
		vmaDestroyImage(Scope->GetAllocator(), object.image, object.memory);
}

const VkImage& GVkImage::GetImage() const
{
	auto& object = _activeObj();
	return object.image;
}

const GVkImageState& GVkImage::_getImageState(uint32_t Index, uint32_t Level, uint32_t Layer) const
{
	auto& object = m_flightResources[Index];
	return object.layouts[Layer][Level];
}

void GVkImage::_setImageState(uint32_t Index, const VkImageSubresourceRange& Range, VkImageLayout Layout, VkPipelineStageFlags Stage, VkAccessFlags Access)
{
	auto& object = m_flightResources[Index];

	uint32_t BaseLevel = Range.baseMipLevel;
	uint32_t LevelCount = Range.levelCount == VK_REMAINING_MIP_LEVELS ? GetMipLevelsCount() - BaseLevel : Range.levelCount;

	uint32_t BaseLayer = m_Type == VK_IMAGE_TYPE_3D ? 0 : Range.baseArrayLayer;
	uint32_t LayerCount = m_Type == VK_IMAGE_TYPE_3D || Range.layerCount == VK_REMAINING_ARRAY_LAYERS ? std::max(GetArrayLayers(), GetExtent().depth) - BaseLayer : Range.layerCount;

	for (uint32_t Layer = BaseLayer; Layer < BaseLayer + LayerCount; Layer++)
	{
		for (uint32_t Level = BaseLevel; Level < BaseLevel + LevelCount; Level++)
		{
			object.layouts[Layer][Level].layout = Layout;
			object.layouts[Layer][Level].access = Access;
			object.layouts[Layer][Level].stage = Stage;
		}
	}
}

size_t GVkImage::GetAllocSize() const
{
	auto& object = _activeObj();

	VmaAllocationInfo info{};
	vmaGetAllocationInfo(Scope->GetAllocator(), object.memory, &info);
	return info.size;
}

std::shared_ptr<GVkImageView> GVkImage::ToView(std::shared_ptr<GVkImage> Image, uint32_t Level, uint32_t Layer, uint32_t Levels, uint32_t Layers, VkImageViewType TypeOverride)
{
	return std::make_shared<GVkImageView>(Image, VkImageSubresourceRange(Image->GetAspect(), Level, Levels, Layer, Layers), TypeOverride);
}

std::shared_ptr<GVkImageView> GVkImage::ToView(std::shared_ptr<GVkImage> Image, VkImageViewType Type)
{
	VkImageSubresourceRange Range = Image->m_SubRange;
	Range.layerCount = VK_REMAINING_ARRAY_LAYERS;
	Range.levelCount = VK_REMAINING_MIP_LEVELS;

	return std::make_shared<GVkImageView>(Image, Range, Type);
}

std::shared_ptr<GVkImageView> GVkImage::ToFlightHadle(std::shared_ptr<GVkImage> Image, int32_t shuffle_offset)
{
	VkImageSubresourceRange Range = Image->m_SubRange;
	Range.layerCount = VK_REMAINING_ARRAY_LAYERS;
	Range.levelCount = VK_REMAINING_MIP_LEVELS;

	return std::make_shared<GVkImageView>(Image, Range, VK_IMAGE_VIEW_TYPE_MAX_ENUM, shuffle_offset);
}

GVkImageView::GVkImageView(std::shared_ptr<GVkImage> Image, VkImageSubresourceRange Range, VkImageViewType Type, int32_t shuffle)
	: m_Image(Image)
	, m_type(Type)
	, m_shuffle(shuffle)
{
	m_Subresource = Range;
	m_flightResources.resize(Image->m_flightResources.size());

	if (m_type == VK_IMAGE_VIEW_TYPE_MAX_ENUM)
		m_type = Image->GetImageType() & VK_IMAGE_TYPE_3D ? VK_IMAGE_VIEW_TYPE_3D : (Image->GetArrayLayers() > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D);

	VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewCreateInfo.subresourceRange = m_Subresource;
	viewCreateInfo.format = Image->GetFormat();
	viewCreateInfo.viewType = m_type;

	int32_t index = 0;
	for (auto& object : m_flightResources)
	{
		viewCreateInfo.image = Image->At(index);
		vkCreateImageView(m_Image->Scope->GetDevice(), &viewCreateInfo, VK_NULL_HANDLE, &object.view);
		index++;
	}
}

GVkImageView::~GVkImageView()
{
	for (auto& object : m_flightResources)
	{
		vkDestroyImageView(m_Image->Scope->GetDevice(), object.view, VK_NULL_HANDLE);
	}
}

void GVkImageView::_setImageState(VkImageLayout Layout, VkPipelineStageFlags Stage, VkAccessFlags Access)
{
	m_Image->_setImageState(_activeIndex(), m_Subresource, Layout, Stage, Access);
}

const GVkImageState& GVkImageView::_getImageState(uint32_t Level, uint32_t Layer)
{
	return m_Image->_getImageState(_activeIndex(), Level, Layer);
}

uint32_t GVkImageView::GetLayerCount() const
{
	uint32_t LayerCount = m_Subresource.layerCount;
	if (LayerCount == VK_REMAINING_ARRAY_LAYERS)
	{
		if (m_type == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
			LayerCount = std::max(m_Image->GetArrayLayers(), m_Image->GetExtent().depth);
		else
			LayerCount = m_Image->GetArrayLayers();
	}

	return LayerCount - m_Subresource.baseArrayLayer;
}

uint32_t GVkImageView::GetLevelCount() const
{
	if (m_Subresource.levelCount == VK_REMAINING_MIP_LEVELS)
		return m_Image->GetMipLevelsCount() - m_Subresource.baseMipLevel;

	return m_Subresource.levelCount;
}

uint32_t GVkImageView::BaseLayer() const
{
	return m_Subresource.baseArrayLayer;
}

uint32_t GVkImageView::LastLayer() const
{
	return BaseLayer() + GetLayerCount();
}

uint32_t GVkImageView::BaseLevel() const
{
	return m_Subresource.baseMipLevel;
}

uint32_t GVkImageView::LastLevel() const
{
	return BaseLevel() + GetLevelCount();
}