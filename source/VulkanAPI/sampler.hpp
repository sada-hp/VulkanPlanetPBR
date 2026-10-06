#pragma once
#include "scope.hpp"

class GVkSampler : public IVkObj
{
private:
	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;

public:
	GVkSampler(std::shared_ptr<RenderScope> Scope, uint32_t MipLevels, VkFilter Filter, VkSamplerAddressMode AddressU, VkSamplerAddressMode AddressV, VkSamplerAddressMode AddressW);
	virtual ~GVkSampler();

	const VkSampler& GetSampler() const { return m_sampler; }
};