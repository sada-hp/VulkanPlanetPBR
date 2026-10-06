#pragma once
#include "buffer.hpp"
#include "image.hpp"

struct GVkSharedResources
{
	// Render targets
	std::shared_ptr<GVkImage> ColorBuffer = VK_NULL_HANDLE;
	// LUT
	std::shared_ptr<GVkImage> TransmittanceLUT = VK_NULL_HANDLE;
	std::shared_ptr<GVkImage> ScatteringLUT = VK_NULL_HANDLE;
	std::shared_ptr<GVkImage> IrradianceLUT = VK_NULL_HANDLE;
	// UBO
	std::shared_ptr<GVkBuffer> UBO = VK_NULL_HANDLE;
};