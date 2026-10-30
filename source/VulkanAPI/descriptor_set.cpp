#include "descriptor_set.hpp"

#define MAX_DESCRIPTOR_SIZE 65384

static uint32_t _clampToCapability(std::shared_ptr<RenderScope> Scope, VkDescriptorType type, uint32_t size)
{
	if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		return std::min(Scope->DeviceLimits().maxPerStageDescriptorSamplers / 2, size);
	else if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		return std::min(Scope->DeviceLimits().maxPerStageDescriptorUniformBuffers / 2, size);
	else if (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		return std::min(Scope->DeviceLimits().maxPerStageDescriptorStorageBuffers / 2, size);
	else if (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		return std::min(Scope->DeviceLimits().maxPerStageDescriptorStorageImages / 2, size);
	else if (type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
		return std::min(Scope->DeviceLimits().maxPerStageDescriptorInputAttachments / 2, size);

	return size;
}

GVkDescriptorSet::GVkDescriptorSet(std::shared_ptr<RenderScope> InScope)
	: Scope(InScope)
{

}

GVkDescriptorSet::~GVkDescriptorSet()
{
	vkDestroyDescriptorPool(Scope->GetDevice(), m_descriptorSetPool, VK_NULL_HANDLE);
}

const VkDescriptorSet& GVkDescriptorSet::GetDescriptorSet() const
{
	auto& object = _activeObj();
	return object.set;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddUniformBuffer(VkShaderStageFlags stages, std::vector<std::shared_ptr<GVkBufferView>> views)
{
	uint32_t arrIndex = 0;
	for (auto& view : views)
	{
		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.descriptorCount = 1;
		write.dstArrayElement = arrIndex;
		write.dstBinding = layoutDescriptor.GetBindingCount();
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bufferWrites.emplace_back(write, view);

		bIsInFlight |= view->GetRoot()->IsInFlight();
		acquiredObjects.push_back(view);
		arrIndex++;
	}

	layoutDescriptor.AddUniformBuffer(stages, true);
	arraySizes.push_back(views.size());

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddUniformBuffer(VkShaderStageFlags stages, std::shared_ptr<GVkBufferView> view)
{
	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.dstArrayElement = 0;
	write.dstBinding = layoutDescriptor.GetBindingCount();
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bufferWrites.emplace_back(write, view);

	bIsInFlight |= view->GetRoot()->IsInFlight();
	acquiredObjects.push_back(view);

	layoutDescriptor.AddUniformBuffer(stages);
	arraySizes.push_back(1);

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddStorageBuffer(VkShaderStageFlags stages, std::vector<std::shared_ptr<GVkBufferView>> views)
{
	uint32_t arrIndex = 0;
	for (auto& view : views)
	{
		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.descriptorCount = 1;
		write.dstArrayElement = arrIndex;
		write.dstBinding = layoutDescriptor.GetBindingCount();
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bufferWrites.emplace_back(write, view);

		bIsInFlight |= view->GetRoot()->IsInFlight();
		acquiredObjects.push_back(view);
		arrIndex++;
	}

	layoutDescriptor.AddStorageBuffer(stages, true);
	arraySizes.push_back(views.size());

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddStorageBuffer(VkShaderStageFlags stages, std::shared_ptr<GVkBufferView> view)
{
	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.dstArrayElement = 0;
	write.dstBinding = layoutDescriptor.GetBindingCount();
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bufferWrites.emplace_back(write, view);

	bIsInFlight |= view->GetRoot()->IsInFlight();
	acquiredObjects.push_back(view);

	layoutDescriptor.AddStorageBuffer(stages);
	arraySizes.push_back(1);

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddImageSampler(VkShaderStageFlags stages, std::shared_ptr<GVkImageView> view, std::shared_ptr<GVkSampler> sampler)
{
	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.dstArrayElement = 0;
	write.dstBinding = layoutDescriptor.GetBindingCount();
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageWrites.emplace_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, write, view, sampler);

	bIsInFlight |= view->GetRoot()->IsInFlight();
	acquiredObjects.push_back(sampler);
	acquiredObjects.push_back(view);

	layoutDescriptor.AddImageSampler(stages);
	arraySizes.push_back(1);

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddImageSampler(VkShaderStageFlags stages, const std::vector<std::pair<std::shared_ptr<GVkImageView>, std::shared_ptr<GVkSampler>>>& image_samplers)
{
	uint32_t arrIndex = 0;
	for (auto& [view, sampler] : image_samplers)
	{
		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.descriptorCount = 1;
		write.dstArrayElement = arrIndex;
		write.dstBinding = layoutDescriptor.GetBindingCount();
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageWrites.emplace_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, write, view, sampler);

		bIsInFlight |= view->GetRoot()->IsInFlight();
		acquiredObjects.push_back(sampler);
		acquiredObjects.push_back(view);
		arrIndex++;
	}

	layoutDescriptor.AddImageSampler(stages, true);
	arraySizes.push_back(image_samplers.size());

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddStorageImage(VkShaderStageFlags stages, std::shared_ptr<GVkImageView> view)
{
	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.dstArrayElement = 0;
	write.dstBinding = layoutDescriptor.GetBindingCount();
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	imageWrites.emplace_back(VK_IMAGE_LAYOUT_GENERAL, write, view, VK_NULL_HANDLE);

	bIsInFlight |= view->GetRoot()->IsInFlight();
	acquiredObjects.push_back(view);

	layoutDescriptor.AddStorageImage(stages);
	arraySizes.push_back(1);

	return *this;
}

DescriptorSetDescriptor& DescriptorSetDescriptor::AddSubpassAttachment(VkShaderStageFlags stages, std::shared_ptr<GVkImageView> view)
{
	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.dstArrayElement = 0;
	write.dstBinding = layoutDescriptor.GetBindingCount();
	write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	imageWrites.emplace_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, write, view, VK_NULL_HANDLE);

	bIsInFlight |= view->GetRoot()->IsInFlight();
	acquiredObjects.push_back(view);

	layoutDescriptor.AddSubpassAttachment(stages);
	arraySizes.push_back(1);

	return *this;
}

std::shared_ptr<GVkDescriptorSet> DescriptorSetDescriptor::Allocate(std::shared_ptr<RenderScope> Scope)
{
	std::shared_ptr<GVkDescriptorSet> descriptor = std::shared_ptr<GVkDescriptorSet>{ new GVkDescriptorSet(Scope) };
	descriptor->m_sharedResources = acquiredObjects;

	auto layout = layoutDescriptor.Construct(Scope);
	descriptor->m_descriptorSetLayout = layout->GetLayout();
	descriptor->m_sharedResources.emplace_back(layout);

	uint32_t FlightCount = bIsInFlight ? Scope->GetMaxFramesInFlight() : 1;

	std::unordered_map<VkDescriptorType, uint32_t> descriptor_counts = {};
	for (auto& binding : layoutDescriptor.bindings)
		descriptor_counts[binding.descriptorType] += binding.descriptorCount;

	std::vector<VkDescriptorPoolSize> pool_sizes = {};
	for (auto& [type, count] : descriptor_counts)
		pool_sizes.emplace_back(type, FlightCount * count);

	VkDescriptorPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolCreateInfo.maxSets = FlightCount;
	poolCreateInfo.poolSizeCount = pool_sizes.size();
	poolCreateInfo.pPoolSizes = pool_sizes.data();
	vkCreateDescriptorPool(Scope->GetDevice(), &poolCreateInfo, VK_NULL_HANDLE, &descriptor->m_descriptorSetPool);

	uint32_t index = 0;
	descriptor->m_flightResources.resize(FlightCount);
	for (auto& object : descriptor->m_flightResources)
	{
		VkDescriptorSetVariableDescriptorCountAllocateInfo variableAllocateInfo = {};
		variableAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
		variableAllocateInfo.descriptorSetCount = arraySizes.size();
		variableAllocateInfo.pDescriptorCounts = arraySizes.data();

		VkDescriptorSetAllocateInfo setAllocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		setAllocInfo.descriptorPool = descriptor->m_descriptorSetPool;
		setAllocInfo.pSetLayouts = &descriptor->m_descriptorSetLayout;
		setAllocInfo.descriptorSetCount = 1;
		setAllocInfo.pNext = layoutDescriptor.bVariable ? &variableAllocateInfo : nullptr;
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
			descriptorInfo.sampler = sampler.get() ? sampler->GetSampler() : VK_NULL_HANDLE;
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

DescriptorLayoutDescriptor& DescriptorLayoutDescriptor::AddUniformBuffer(VkShaderStageFlags stages, bool bVariableSize)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindingInfo.descriptorCount = bVariableSize ? MAX_DESCRIPTOR_SIZE : 1;
	bindingInfo.binding = bindings.size();
	bindingInfo.stageFlags = stages;
	bindings.push_back(bindingInfo);

	flags.push_back(bVariableSize ? variableFlags : 0);
	bVariable = bVariable || bVariableSize;

	return *this;
}

DescriptorLayoutDescriptor& DescriptorLayoutDescriptor::AddStorageBuffer(VkShaderStageFlags stages, bool bVariableSize)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindingInfo.descriptorCount = bVariableSize ? MAX_DESCRIPTOR_SIZE : 1;
	bindingInfo.binding = bindings.size();
	bindingInfo.stageFlags = stages;
	bindings.push_back(bindingInfo);

	flags.push_back(bVariableSize ? variableFlags : 0);
	bVariable = bVariable || bVariableSize;

	return *this;
}

DescriptorLayoutDescriptor& DescriptorLayoutDescriptor::AddImageSampler(VkShaderStageFlags stages, bool bVariableSize)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindingInfo.descriptorCount = bVariableSize ? MAX_DESCRIPTOR_SIZE : 1;
	bindingInfo.binding = bindings.size();
	bindingInfo.stageFlags = stages;
	bindings.push_back(bindingInfo);

	flags.push_back(bVariableSize ? variableFlags : 0);
	bVariable = bVariable || bVariableSize;

	return *this;
}

DescriptorLayoutDescriptor& DescriptorLayoutDescriptor::AddStorageImage(VkShaderStageFlags stages, bool bVariableSize)
{
	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindingInfo.descriptorCount = bVariableSize ? MAX_DESCRIPTOR_SIZE : 1;
	bindingInfo.binding = bindings.size();
	bindingInfo.stageFlags = stages;
	bindings.push_back(bindingInfo);

	flags.push_back(bVariableSize ? variableFlags : 0);
	bVariable = bVariable || bVariableSize;

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
	flags.push_back(0);
	return *this;
}

std::shared_ptr<GVkDescriptorSetLayout> DescriptorLayoutDescriptor::Construct(std::shared_ptr<RenderScope> Scope)
{
	for (auto& binding : bindings)
		binding.descriptorCount = _clampToCapability(Scope, binding.descriptorType, binding.descriptorCount);

	VkDescriptorSetLayoutBindingFlagsCreateInfo layoutFlagsInfo = {};
	layoutFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	layoutFlagsInfo.pBindingFlags = flags.data();
	layoutFlagsInfo.bindingCount = flags.size();

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutCreateInfo.bindingCount = bindings.size();
	layoutCreateInfo.pBindings = bindings.data();
	layoutCreateInfo.pNext = &layoutFlagsInfo;

	return std::shared_ptr<GVkDescriptorSetLayout>(new GVkDescriptorSetLayout(Scope, layoutCreateInfo));
}