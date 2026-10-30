#pragma once
#include "descriptor_set.hpp"
#include "buffer.hpp"
#include "image.hpp"

struct GVkSharedResources
{
private:
	GVkSharedResources() = default;

public:
	// Render targets
	std::shared_ptr<GVkImage> ColorBuffer = VK_NULL_HANDLE;
	std::shared_ptr<GVkImage> DepthBuffer = VK_NULL_HANDLE;
	std::shared_ptr<GVkImage> FinalTarget = VK_NULL_HANDLE;
	// LUT
	std::shared_ptr<GVkImage> TransmittanceLUT = VK_NULL_HANDLE;
	std::shared_ptr<GVkImage> ScatteringLUT = VK_NULL_HANDLE;
	std::shared_ptr<GVkImage> IrradianceLUT = VK_NULL_HANDLE;
	// UBO
	std::shared_ptr<GVkBuffer> UBO = VK_NULL_HANDLE;

public:
	// Instance
	static GVkSharedResources Instance;
};