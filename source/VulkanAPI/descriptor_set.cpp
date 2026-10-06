#include "descriptor_set.hpp"

GVkDescriptorSet::GVkDescriptorSet(std::shared_ptr<RenderScope> InScope)
	: Scope(InScope)
{

}

GVkDescriptorSet::~GVkDescriptorSet()
{
	vkDestroyDescriptorSetLayout(Scope->GetDevice(), m_descriptorSetLayout, VK_NULL_HANDLE);
	vkDestroyDescriptorPool(Scope->GetDevice(), m_descriptorSetPool, VK_NULL_HANDLE);
}

void GVkDescriptorSet::_addResource(std::shared_ptr<IVkObj> resource)
{
	if (resource)
		m_sharedResources.push_back(resource);
}

const VkDescriptorSet& GVkDescriptorSet::GetDescriptorSet() const
{
	auto& object = _activeObj();
	return object.set;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddUniformBuffer(VkShaderStageFlags stages, std::shared_ptr<GVkBufferView> view)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindingInfo.binding = bindingCount;
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;

	bufferResources.emplace_back(bindingInfo, view);
	bIsInFlight |= view->GetRoot()->IsInFlight();
	bindingCount++;

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddStorageBuffer(VkShaderStageFlags stages, std::shared_ptr<GVkBufferView> view)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindingInfo.binding = bindingCount;
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;

	bufferResources.emplace_back(bindingInfo, view);
	bIsInFlight |= view->GetRoot()->IsInFlight();
	bindingCount++;

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddImageSampler(VkShaderStageFlags stages, std::shared_ptr<GVkImageView> view, std::shared_ptr<GVkSampler> sampler)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindingInfo.binding = bindingCount;
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;

	imageResources.emplace_back(bindingInfo, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler);
	bIsInFlight |= view->GetRoot()->IsInFlight();
	bindingCount++;

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddStorageImage(VkShaderStageFlags stages, std::shared_ptr<GVkImageView> view)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindingInfo.binding = bindingCount;
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;

	imageResources.emplace_back(bindingInfo, view, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE);
	bIsInFlight |= view->GetRoot()->IsInFlight();
	bindingCount++;

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddSubpassAttachment(VkShaderStageFlags stages, std::shared_ptr<GVkImageView> view)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	bindingInfo.binding = bindingCount;
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;

	imageResources.emplace_back(bindingInfo, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_NULL_HANDLE);
	bIsInFlight |= view->GetRoot()->IsInFlight();
	bindingCount++;

	return *this;
}

std::shared_ptr<GVkDescriptorSet> DescriptorSetDescriptor::Allocate(std::shared_ptr<RenderScope> Scope)
{
	std::shared_ptr<GVkDescriptorSet> descriptor = std::shared_ptr<GVkDescriptorSet>{ new GVkDescriptorSet(Scope) };

	uint32_t FlightCount = bIsInFlight ? Scope->GetMaxFramesInFlight() : 1;
	std::unordered_map<VkDescriptorType, uint32_t> descriptor_counts = {};
	std::vector<VkDescriptorSetLayoutBinding> descriptor_bindings = {};
	std::vector<VkDescriptorPoolSize> pool_sizes = {};

	for (auto& [binding, view, layout, sampler] : imageResources)
	{
		descriptor_counts[binding.descriptorType] += FlightCount * binding.descriptorCount;

		descriptor_bindings.push_back(binding);
		descriptor->_addResource(sampler);
		descriptor->_addResource(view);
	}

	for (auto& [binding, view] : bufferResources)
	{
		descriptor_counts[binding.descriptorType] += FlightCount * binding.descriptorCount;

		descriptor_bindings.push_back(binding);
		descriptor->_addResource(view);
	}

	for (auto& [type, count] : descriptor_counts)
		pool_sizes.emplace_back(type, count);

	VkDescriptorPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolCreateInfo.maxSets = FlightCount;
	poolCreateInfo.poolSizeCount = pool_sizes.size();
	poolCreateInfo.pPoolSizes = pool_sizes.data();
	vkCreateDescriptorPool(Scope->GetDevice(), &poolCreateInfo, VK_NULL_HANDLE, &descriptor->m_descriptorSetPool);

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutCreateInfo.bindingCount = descriptor_bindings.size();
	layoutCreateInfo.pBindings = descriptor_bindings.data();
	vkCreateDescriptorSetLayout(Scope->GetDevice(), &layoutCreateInfo, VK_NULL_HANDLE, &descriptor->m_descriptorSetLayout);

	uint32_t index = 0;
	descriptor->m_flightResources.resize(FlightCount);
	for (auto& object : descriptor->m_flightResources)
	{
		VkDescriptorSetAllocateInfo setAllocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		setAllocInfo.descriptorPool = descriptor->m_descriptorSetPool;
		setAllocInfo.pSetLayouts = &descriptor->m_descriptorSetLayout;
		setAllocInfo.descriptorSetCount = 1;
		auto res = vkAllocateDescriptorSets(Scope->GetDevice(), &setAllocInfo, &object.set);
		assert(res == VK_SUCCESS);

		std::vector<VkWriteDescriptorSet> descriptor_writes = {};
		std::vector<VkDescriptorBufferInfo> buffer_infos = {};
		std::vector<VkDescriptorImageInfo> image_infos = {};

		image_infos.reserve(imageResources.size());
		buffer_infos.reserve(bufferResources.size());

		for (auto& [binding, view, layout, sampler] : imageResources)
		{
			VkDescriptorImageInfo descriptorInfo{};
			descriptorInfo.imageLayout = layout;
			descriptorInfo.imageView = view->At(index);
			descriptorInfo.sampler = sampler ? sampler->GetSampler() : VK_NULL_HANDLE;
			image_infos.push_back(descriptorInfo);

			VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			write.descriptorCount = binding.descriptorCount;
			write.descriptorType = binding.descriptorType;
			write.pImageInfo = &image_infos.back();
			write.dstBinding = binding.binding;
			write.dstSet = object.set;
			descriptor_writes.push_back(write);
		}

		for (auto& [binding, view] : bufferResources)
		{
			VkDescriptorBufferInfo descriptorInfo{};
			descriptorInfo.buffer = view->At(index);
			descriptorInfo.offset = view->GetOffset();
			descriptorInfo.range = view->GetSize();
			buffer_infos.push_back(descriptorInfo);

			VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			write.descriptorCount = binding.descriptorCount;
			write.descriptorType = binding.descriptorType;
			write.pBufferInfo = &buffer_infos.back();
			write.dstSet = object.set;
			descriptor_writes.push_back(write);
		}

		vkUpdateDescriptorSets(Scope->GetDevice(), descriptor_writes.size(), descriptor_writes.data(), 0, VK_NULL_HANDLE);
		index++;
	}

	return descriptor;
}

GVkDescriptorSetLayout::GVkDescriptorSetLayout(std::shared_ptr<RenderScope> InScope, const VkDescriptorSetLayoutCreateInfo& info)
	: Scope(InScope)
{
	vkCreateDescriptorSetLayout(Scope->GetDevice(), &info, VK_NULL_HANDLE, &m_descriptorSetLayout);
}

GVkDescriptorSetLayout::~GVkDescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(Scope->GetDevice(), m_descriptorSetLayout, VK_NULL_HANDLE);
}

DescriptorLayoutDescriptor& DescriptorLayoutDescriptor::AddUniformBuffer(VkShaderStageFlags stages)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindingInfo.binding = bindings.size();
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;
	bindings.push_back(bindingInfo);
	return *this;
}

DescriptorLayoutDescriptor& DescriptorLayoutDescriptor::AddStorageBuffer(VkShaderStageFlags stages)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindingInfo.binding = bindings.size();
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;
	bindings.push_back(bindingInfo);
	return *this;
}

DescriptorLayoutDescriptor& DescriptorLayoutDescriptor::AddImageSampler(VkShaderStageFlags stages)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindingInfo.binding = bindings.size();
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;
	bindings.push_back(bindingInfo);
	return *this;
}

DescriptorLayoutDescriptor& DescriptorLayoutDescriptor::AddStorageImage(VkShaderStageFlags stages)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindingInfo.binding = bindings.size();
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;
	bindings.push_back(bindingInfo);
	return *this;
}

DescriptorLayoutDescriptor& DescriptorLayoutDescriptor::AddSubpassAttachment(VkShaderStageFlags stages)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	bindingInfo.binding = bindings.size();
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;
	bindings.push_back(bindingInfo);
	return *this;
}

std::shared_ptr<GVkDescriptorSetLayout> DescriptorLayoutDescriptor::Construct(std::shared_ptr<RenderScope> Scope)
{
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutCreateInfo.bindingCount = bindings.size();
	layoutCreateInfo.pBindings = bindings.data();
	return std::shared_ptr<GVkDescriptorSetLayout>(new GVkDescriptorSetLayout(Scope, layoutCreateInfo));
}