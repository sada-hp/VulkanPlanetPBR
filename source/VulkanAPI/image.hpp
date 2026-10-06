#pragma once
#include "scope.hpp"

enum class EImageFlags
{
	Sampler = 0,
	Storage         = Bit(1),
	RenderTarget    = Bit(2),
	TransferTarget  = Bit(3),
	TransferSource  = Bit(4),
	SubpassInput    = Bit(5),
	AllocateMipMaps = Bit(6),
	DepthAsLayers   = Bit(7),
	Cubemap         = Bit(8),
	InFlight        = Bit(9)
};

DefineFlags(EImageFlags)

struct GVkImageState
{
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkPipelineStageFlags2 stage = 0;
	VkAccessFlags2 access = 0;
};

class GVkImageView;

class GVkImage : public IVkObj
{
	friend class GVkCommandBuffer;
	friend class GVkImageView;
	friend class GVkBarrier;

	struct _internalObj
	{
		VkImage image = VK_NULL_HANDLE;
		VmaAllocation memory = VK_NULL_HANDLE;
		std::vector<std::vector<GVkImageState>> layouts = {};
	};

private:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	std::vector<_internalObj> m_flightResources = {};

	VkImageSubresourceRange m_SubRange = {};
	VkExtent3D m_Extents  = {};
	VkFormat m_Format  = {};
	VkImageType m_Type = {};

private:
	size_t _activeIndex() const { return Scope->GetResourceIndex() % m_flightResources.size(); }
	const _internalObj& _activeObj() const { return m_flightResources.at(_activeIndex()); }
	_internalObj& _activeObj() { return m_flightResources.at(_activeIndex()); }

protected:
	const GVkImageState& _getImageState(uint32_t Level, uint32_t Layer = 0) const;
	void _setImageState(uint32_t Level, uint32_t Layer, VkImageLayout Layout, VkPipelineStageFlags Stage, VkAccessFlags Access);
	void _setImageState(const VkImageSubresourceRange& Range, VkImageLayout Layout, VkPipelineStageFlags Stage, VkAccessFlags Access);

public:
	inline const VkImage& operator[](size_t index) const { return m_flightResources.at(index % m_flightResources.size()).image; }

public:
	GVkImage(std::shared_ptr<RenderScope> Scope, VkFormat Format, VkExtent3D Extents, EImageFlags Flags);
	virtual ~GVkImage();

public:
	size_t GetBytesSize() const;
	const VkImage& GetImage() const;
	
	bool IsInFlight() const { return m_flightResources.size() != 1; }
	const VkImage& At(size_t Index) const { return (*this)[Index]; }

	bool IsDepth() const { return m_SubRange.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT; }
	const VkImageAspectFlags& GetAspect() const { return m_SubRange.aspectMask; };
	const VkImageType GetImageType() const { return m_Type; };
	const VkFormat& GetFormat() const { return m_Format; };
	
	uint32_t GetMipLevelsCount() const { return m_SubRange.levelCount; };
	uint32_t GetArrayLayers() const { return m_SubRange.layerCount; };

	VkExtent3D GetExtent(uint32_t Level = 0) const { return VkExtent3D{ m_Extents.width >> Level, m_Extents.height >> Level, m_Extents.depth }; };

	static std::shared_ptr<GVkImageView> ToView(std::shared_ptr<GVkImage> Image, uint32_t Level = 0, uint32_t Layer = 0, uint32_t Levels = VK_REMAINING_MIP_LEVELS, uint32_t Layers = VK_REMAINING_ARRAY_LAYERS, VkImageViewType TypeOverride = VK_IMAGE_VIEW_TYPE_MAX_ENUM);
	static std::shared_ptr<GVkImageView> ToView(std::shared_ptr<GVkImage> Image, VkImageViewType Type);
};

class GVkImageView : public IVkObj
{
	friend class GVkCommandBuffer;
	friend class GVkBarrier;

	struct _internalObj
	{
		VkImageView view = VK_NULL_HANDLE;
	};

private:
	std::shared_ptr<GVkImage> m_Image = VK_NULL_HANDLE;
	std::vector<_internalObj> m_flightResources = {};
	VkImageSubresourceRange m_Subresource;
	VkImageViewType m_type;

protected:
	void _setImageState(VkImageLayout Layout, VkPipelineStageFlags Stage, VkAccessFlags Access);

public:
	GVkImageView(std::shared_ptr<GVkImage> Image, VkImageSubresourceRange Range, VkImageViewType Type);
	~GVkImageView();

	const VkImageView& At(size_t index) const { return m_flightResources[index % m_flightResources.size()].view; }
	const VkImageView& GetImageView() const { return m_flightResources[m_Image->_activeIndex()].view; }
	const VkImage& GetImage() const { return m_Image->GetImage(); }
	const VkImageViewType GetType() const { return m_type; }

	uint32_t GetLayerCount() const;
	uint32_t GetLevelCount() const;

	uint32_t BaseLayer() const;
	uint32_t LastLayer() const;

	uint32_t BaseLevel() const;
	uint32_t LastLevel() const;

	const VkImageSubresourceRange& GetSubresource() const { return m_Subresource; }
	std::shared_ptr<GVkImage> GetRoot() const { return m_Image; }
};