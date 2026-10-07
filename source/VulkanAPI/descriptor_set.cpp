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

const VkDescriptorSet& GVkDescriptorSet::GetDescriptorSet() const
{
	auto& object = _activeObj();
	return object.set;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddUniformBuffer(VkShaderStageFlags stages, std::shared_ptr<GVkBufferView> view)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindingInfo.binding = descriptorBindings.size();
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;
	descriptorBindings.push_back(bindingInfo);

	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.dstArrayElement = 0;
	write.dstBinding = bindingInfo.binding;
	write.descriptorType = bindingInfo.descriptorType;
	bufferWrites.emplace_back(write, view);

	bIsInFlight |= view->GetRoot()->IsInFlight();
	acquiredObjects.push_back(view);

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddStorageBuffer(VkShaderStageFlags stages, std::shared_ptr<GVkBufferView> view)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindingInfo.binding = descriptorBindings.size();
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;
	descriptorBindings.push_back(bindingInfo);

	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.dstArrayElement = 0;
	write.dstBinding = bindingInfo.binding;
	write.descriptorType = bindingInfo.descriptorType;
	bufferWrites.emplace_back(write, view);

	bIsInFlight |= view->GetRoot()->IsInFlight();
	acquiredObjects.push_back(view);

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddImageSampler(VkShaderStageFlags stages, std::shared_ptr<GVkImageView> view, std::shared_ptr<GVkSampler> sampler)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindingInfo.binding = descriptorBindings.size();
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;
	descriptorBindings.push_back(bindingInfo);

	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.dstArrayElement = 0;
	write.dstBinding = bindingInfo.binding;
	write.descriptorType = bindingInfo.descriptorType;
	imageWrites.emplace_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, write, view, sampler);

	bIsInFlight |= view->GetRoot()->IsInFlight();
	acquiredObjects.push_back(sampler);
	acquiredObjects.push_back(view);

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddImageSampler(VkShaderStageFlags stages, const std::vector<std::pair<std::shared_ptr<GVkImageView>, std::shared_ptr<GVkSampler>>>& image_samplers)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindingInfo.descriptorCount = image_samplers.size();
	bindingInfo.binding = descriptorBindings.size();
	bindingInfo.stageFlags = stages;
	descriptorBindings.push_back(bindingInfo);

	uint32_t arrIndex = 0;
	for (auto& [view, sampler] : image_samplers)
	{
		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.descriptorCount = 1;
		write.dstArrayElement = arrIndex;
		write.dstBinding = bindingInfo.binding;
		write.descriptorType = bindingInfo.descriptorType;
		imageWrites.emplace_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, write, view, sampler);

		bIsInFlight |= view->GetRoot()->IsInFlight();
		acquiredObjects.push_back(sampler);
		acquiredObjects.push_back(view);
		arrIndex++;
	}

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddStorageImage(VkShaderStageFlags stages, std::shared_ptr<GVkImageView> view)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindingInfo.binding = descriptorBindings.size();
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;
	descriptorBindings.push_back(bindingInfo);

	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.dstArrayElement = 0;
	write.dstBinding = bindingInfo.binding;
	write.descriptorType = bindingInfo.descriptorType;
	imageWrites.emplace_back(VK_IMAGE_LAYOUT_GENERAL, write, view, VK_NULL_HANDLE);

	bIsInFlight |= view->GetRoot()->IsInFlight();
	acquiredObjects.push_back(view);

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddSubpassAttachment(VkShaderStageFlags stages, std::shared_ptr<GVkImageView> view)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	bindingInfo.binding = descriptorBindings.size();
	bindingInfo.stageFlags = stages;
	bindingInfo.descriptorCount = 1;
	descriptorBindings.push_back(bindingInfo);

	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.dstArrayElement = 0;
	write.dstBinding = bindingInfo.binding;
	write.descriptorType = bindingInfo.descriptorType;
	imageWrites.emplace_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, write, view, VK_NULL_HANDLE);

	bIsInFlight |= view->GetRoot()->IsInFlight();
	acquiredObjects.push_back(view);

	return *this;
}

std::shared_ptr<GVkDescriptorSet> DescriptorSetDescriptor::Allocate(std::shared_ptr<RenderScope> Scope)
{
	std::shared_ptr<GVkDescriptorSet> descriptor = std::shared_ptr<GVkDescriptorSet>{ new GVkDescriptorSet(Scope) };
	descriptor->m_sharedResources = acquiredObjects;

	uint32_t FlightCount = bIsInFlight ? Scope->GetMaxFramesInFlight() : 1;

	std::unordered_map<VkDescriptorType, uint32_t> descriptor_counts = {};
	for (auto binding : descriptorBindings)
		descriptor_counts[binding.descriptorType] += FlightCount * binding.descriptorCount;

	std::vector<VkDescriptorPoolSize> pool_sizes = {};
	for (auto& [type, count] : descriptor_counts)
		pool_sizes.emplace_back(type, count);

	VkDescriptorPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolCreateInfo.maxSets = FlightCount;
	poolCreateInfo.poolSizeCount = pool_sizes.size();
	poolCreateInfo.pPoolSizes = pool_sizes.data();
	vkCreateDescriptorPool(Scope->GetDevice(), &poolCreateInfo, VK_NULL_HANDLE, &descriptor->m_descriptorSetPool);

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutCreateInfo.bindingCount = descriptorBindings.size();
	layoutCreateInfo.pBindings = descriptorBindings.data();
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

		buffer_infos.reserve(bufferWrites.size());
		image_infos.reserve(imageWrites.size());

		for (auto& [write, view] : bufferWrites)
		{
			VkDescriptorBufferInfo descriptorInfo{};
			descriptorInfo.buffer = view->At(index);
			descriptorInfo.offset = view->GetOffset();
			descriptorInfo.range = view->GetSize();
			buffer_infos.push_back(descriptorInfo);

			write.dstSet = object.set;
			write.pBufferInfo = &buffer_infos.back();
			descriptor_writes.push_back(write);
		}

		for (auto& [layout, write, view, sampler] : imageWrites)
		{
			VkDescriptorImageInfo descriptorInfo{};
			descriptorInfo.imageView = view->At(index);
			descriptorInfo.imageLayout = layout;
			descriptorInfo.sampler = sampler->GetSampler();
			image_infos.push_back(descriptorInfo);

			write.dstSet = object.set;
			write.pImageInfo = &image_infos.back();
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