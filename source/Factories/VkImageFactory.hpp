#pragma once
#include "VulkanAPI/command_buffer.hpp"

namespace GVkImageFactory
{
	inline std::shared_ptr<GVkImage> Image(std::shared_ptr<RenderScope> Scope, VkFormat Format, VkExtent3D Extents, EImageFlags Flags, void* Pixels = nullptr)
	{
		std::shared_ptr<GVkImage> _image = std::make_shared<GVkImage>(Scope, Format, Extents, Flags);
		std::shared_ptr<GVkCommandBuffer> _cmd = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_GRAPHICS_BIT);

		if (Pixels)
		{
			auto _buffer = std::make_shared<GVkBuffer>(Scope, _image->GetBytesSize(), EBufferFlags::Mapped | EBufferFlags::TransferSrc);
			_cmd->UpdateBuffer(GVkBuffer::ToView(_buffer), (char*)Pixels);

			if (CheckFlag(Flags, EImageFlags::AllocateMipMaps))
			{

			}
		}
		
		_cmd->BindBarrier<GVkShaderReadBarrier>(GVkImage::ToView(_image), VK_SHADER_STAGE_FRAGMENT_BIT);
		_cmd->Submit();

		return _image;
	}

	inline std::shared_ptr<GVkImage> ColorRenderTargetFlight(std::shared_ptr<RenderScope> Scope, VkExtent2D Extents)
	{
		return Image(Scope, RenderScope::GetColorFormat(), {Extents.width, Extents.height, 1}, EImageFlags::InFlight | EImageFlags::Sampler | EImageFlags::RenderTarget | EImageFlags::TransferTarget | EImageFlags::TransferSource);
	}

	inline std::shared_ptr<GVkImage> DepthRenderTargetFlight(std::shared_ptr<RenderScope> Scope, VkExtent2D Extents)
	{
		return Image(Scope, RenderScope::GetDepthFormat(), {Extents.width, Extents.height, 1}, EImageFlags::InFlight | EImageFlags::Sampler | EImageFlags::RenderTarget | EImageFlags::TransferTarget);
	}

	inline std::shared_ptr<GVkImage> ReadOnlySampler2D(std::shared_ptr<RenderScope> Scope, VkExtent2D Extents, void* Pixels)
	{
		return Image(Scope, VK_FORMAT_R8G8B8A8_UNORM, { Extents.width, Extents.height, 1 }, EImageFlags::Sampler | EImageFlags::AllocateMipMaps | EImageFlags::TransferTarget, Pixels);
	}
};