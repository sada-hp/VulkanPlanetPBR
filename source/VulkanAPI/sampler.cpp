#include "sampler.hpp"

GVkSampler::GVkSampler(std::shared_ptr<RenderScope> InScope, uint32_t MipLevels, VkFilter Filter, VkSamplerAddressMode AddressU, VkSamplerAddressMode AddressV, VkSamplerAddressMode AddressW)
	: Scope(InScope)
{
	VkPhysicalDeviceProperties deviceProperties{};
	vkGetPhysicalDeviceProperties(Scope->GetPhysicalDevice(), &deviceProperties);

	VkSamplerCreateInfo samplerCreateInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCreateInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.addressModeU = AddressU;
	samplerCreateInfo.addressModeV = AddressV;
	samplerCreateInfo.addressModeW = AddressW;
	samplerCreateInfo.magFilter = Filter;
	samplerCreateInfo.minFilter = Filter;
	samplerCreateInfo.maxLod = MipLevels;

	vkCreateSampler(Scope->GetDevice(), &samplerCreateInfo, VK_NULL_HANDLE, &m_sampler);
}

GVkSampler::~GVkSampler()
{
	vkDestroySampler(Scope->GetDevice(), m_sampler, VK_NULL_HANDLE);
}