#pragma once
#include "VulkanAPI/descriptor_set.hpp"
#include "VulkanAPI/pipeline.hpp"
#include "VulkanAPI/scope.hpp"

struct TexturePack
{
	std::shared_ptr<GVkImage> Albedo = VK_NULL_HANDLE;
};

struct MaterialDescriptor
{
public:
	VkCullModeFlagBits CullMode = VK_CULL_MODE_BACK_BIT;
	VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	VkPrimitiveTopology PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;

public:
	std::vector<TexturePack> SubmeshTextures;
};

class IMaterial
{
protected:
	std::shared_ptr<RenderScope> Scope;
	std::shared_ptr<GVkPipeline> Pipeline;
	std::shared_ptr<GVkDescriptorSet> DescriptorSet;

public:
	IMaterial(std::shared_ptr<RenderScope> InScope)
		: Scope(InScope)
	{
	}

	virtual ~IMaterial()
	{
	}

	std::shared_ptr<GVkPipeline> GetPipeline() const { return Pipeline; }
	std::shared_ptr<GVkDescriptorSet> GetDescriptorSet() const { return DescriptorSet; }
};