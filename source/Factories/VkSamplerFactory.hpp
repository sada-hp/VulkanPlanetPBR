#pragma once
#include "VulkanAPI/sampler.hpp"
#include "VulkanAPI/scope.hpp"

namespace GVkSamplerFactory
{
	inline std::shared_ptr<GVkSampler> Sampler(std::shared_ptr<RenderScope> Scope, uint32_t MipLevels, VkFilter Filter, VkSamplerAddressMode AddressU, VkSamplerAddressMode AddressV, VkSamplerAddressMode AddressW)
	{
		return std::make_shared<GVkSampler>(Scope, MipLevels, Filter, AddressU, AddressV, AddressW);
	}

	inline std::shared_ptr<GVkSampler> LinearSamplerRepeat(std::shared_ptr<RenderScope> Scope, uint32_t MipLevels = 1)
	{
		return Sampler(Scope, MipLevels, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	}

	inline std::shared_ptr<GVkSampler> LinearSamplerClamp(std::shared_ptr<RenderScope> Scope, uint32_t MipLevels = 1)
	{
		return Sampler(Scope, MipLevels, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	}

	inline std::shared_ptr<GVkSampler> PointSamplerRepeat(std::shared_ptr<RenderScope> Scope, uint32_t MipLevels = 1)
	{
		return Sampler(Scope, MipLevels, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	}

	inline std::shared_ptr<GVkSampler> PointSamplerClamp(std::shared_ptr<RenderScope> Scope, uint32_t MipLevels = 1)
	{
		return Sampler(Scope, MipLevels, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	}
};