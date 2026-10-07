#pragma once
#include "pipeline.hpp"
#include "sampler.hpp"
#include "buffer.hpp"
#include "image.hpp"
#include "scope.hpp"

class GVkDescriptorSet : public IVkObj
{
	friend class DescriptorSetDescriptor;
	
	struct _internalObj
	{
		VkDescriptorSet set;
	};

private:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;

	std::vector<std::shared_ptr<IVkObj>> m_sharedResources = {};
	std::vector<_internalObj> m_flightResources = {};

	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptorSetPool = VK_NULL_HANDLE;

private:
	size_t _activeIndex() const { return Scope->GetResourceIndex() % m_flightResources.size(); }
	const _internalObj& _activeObj() const { return m_flightResources.at(_activeIndex()); }
	_internalObj& _activeObj() { return m_flightResources.at(_activeIndex()); }

private:
	GVkDescriptorSet(std::shared_ptr<RenderScope> Scope);

public:
	virtual ~GVkDescriptorSet();

	const VkDescriptorSetLayout& GetLayout() const { return m_descriptorSetLayout; };
	const VkDescriptorSet& GetDescriptorSet() const;
};

class DescriptorSetDescriptor
{
	struct _buffer_write
	{
		VkWriteDescriptorSet writeInfo;
		std::shared_ptr<GVkBufferView> view;
	};

	struct _image_write
	{
		VkImageLayout layout;
		VkWriteDescriptorSet writeInfo;
		std::shared_ptr<GVkImageView> imageView;
		std::shared_ptr<GVkSampler> imageSampler;
	};

private:
	std::vector<VkDescriptorSetLayoutBinding> descriptorBindings = {};
	std::vector<std::shared_ptr<IVkObj>> acquiredObjects = {};
	std::vector<_buffer_write> bufferWrites = {};
	std::vector<_image_write> imageWrites = {};

	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	bool bIsInFlight = false;

public:
	DescriptorSetDescriptor& AddUniformBuffer(VkShaderStageFlags stages, std::shared_ptr<GVkBufferView> view);
	DescriptorSetDescriptor& AddStorageBuffer(VkShaderStageFlags stages, std::shared_ptr<GVkBufferView> view);

	DescriptorSetDescriptor& AddImageSampler(VkShaderStageFlags stages, const std::vector<std::pair<std::shared_ptr<GVkImageView>, std::shared_ptr<GVkSampler>>>& image_samplers);
	DescriptorSetDescriptor& AddImageSampler(VkShaderStageFlags stages, std::shared_ptr<GVkImageView> view, std::shared_ptr<GVkSampler> sampler);
	DescriptorSetDescriptor& AddStorageImage(VkShaderStageFlags stages, std::shared_ptr<GVkImageView> view);

	DescriptorSetDescriptor& AddSubpassAttachment(VkShaderStageFlags stages, std::shared_ptr<GVkImageView> view);

	std::shared_ptr<GVkDescriptorSet> Allocate(std::shared_ptr<RenderScope> Scope);
};

class GVkDescriptorSetLayout : public IVkObj
{
	friend class DescriptorLayoutDescriptor;

private:
	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;

private:
	GVkDescriptorSetLayout(std::shared_ptr<RenderScope> Scope, const VkDescriptorSetLayoutCreateInfo& info);

public:
	virtual ~GVkDescriptorSetLayout();

	const VkDescriptorSetLayout& GetLayout() const { return m_descriptorSetLayout; }
};

class DescriptorLayoutDescriptor
{
	std::vector<VkDescriptorSetLayoutBinding> bindings = {};

public:
	DescriptorLayoutDescriptor& AddUniformBuffer(VkShaderStageFlags stages);
	DescriptorLayoutDescriptor& AddStorageBuffer(VkShaderStageFlags stages);

	DescriptorLayoutDescriptor& AddImageSampler(VkShaderStageFlags stages);
	DescriptorLayoutDescriptor& AddStorageImage(VkShaderStageFlags stages);

	DescriptorLayoutDescriptor& AddSubpassAttachment(VkShaderStageFlags stages);

	std::shared_ptr<GVkDescriptorSetLayout> Construct(std::shared_ptr<RenderScope> Scope);
};