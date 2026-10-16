#pragma once
#include "VulkanAPI/command_buffer.hpp"

#define COLOR(r, g, b) (uint32_t(r & 0xff)) | (uint32_t(g & 0xff) << 8) | (uint32_t(b & 0xff) << 16) | (255u << 24)

namespace GVkImageFactory
{
	inline std::shared_ptr<GVkImage> Image(std::shared_ptr<RenderScope> Scope, VkFormat Format, VkExtent3D Extents, EImageFlags Flags, void* Pixels = nullptr, size_t size = 0)
	{
		std::shared_ptr<GVkImage> _image = std::make_shared<GVkImage>(Scope, Format, Extents, Flags | EImageFlags::TransferTarget);
		std::shared_ptr<GVkCommandBuffer> _cmd = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_GRAPHICS_BIT);

		std::shared_ptr<GVkBuffer> stagedBuffer = VK_NULL_HANDLE;

		if (Pixels && size)
		{
			stagedBuffer = std::make_shared<GVkBuffer>(Scope, _image->GetAllocSize(), EBufferFlags::Mapped | EBufferFlags::TransferSrc);
			_cmd->UpdateBuffer(GVkBuffer::ToView(stagedBuffer, 0, size), (char*)Pixels);
		}

		for (uint32_t i = 0; i < _image->FlightCount(); i++)
		{
			auto view = GVkImage::ToFlightHadle(_image, i);

			if (stagedBuffer)
			{
				_cmd->CopyBuffer(GVkBuffer::ToView(stagedBuffer), view);

				if (CheckFlag(Flags, EImageFlags::AllocateMipMaps) && _image->GetMipLevelsCount() > 1)
					_cmd->GenerateMipMaps(_image);
			}
			else
			{
				if (_image->IsDepth())
					_cmd->ClearImage(view, VkClearDepthStencilValue{});
				else
					_cmd->ClearImage(view, VkClearColorValue{});
			}

			_cmd->BindBarrier<GVkShaderReadBarrier>(view, VK_SHADER_STAGE_FRAGMENT_BIT);
		}

		_cmd->Submit();

		return _image;
	}

	inline std::shared_ptr<GVkImage> FinalRenderTargetFlight(std::shared_ptr<RenderScope> Scope, VkExtent2D Extents)
	{
		return Image(Scope, VK_FORMAT_R8G8B8A8_UNORM, { Extents.width, Extents.height, 1 }, EImageFlags::InFlight | EImageFlags::Sampler | EImageFlags::RenderTarget | EImageFlags::TransferSource);
	}

	inline std::shared_ptr<GVkImage> RenderTargetFlight(std::shared_ptr<RenderScope> Scope, VkFormat Format, VkExtent2D Extents)
	{
		return Image(Scope, Format, { Extents.width, Extents.height, 1 }, EImageFlags::InFlight | EImageFlags::Sampler | EImageFlags::RenderTarget | EImageFlags::TransferSource | EImageFlags::TransferTarget);
	}

	inline std::shared_ptr<GVkImage> ColorRenderTargetFlight(std::shared_ptr<RenderScope> Scope, VkExtent2D Extents)
	{
		return RenderTargetFlight(Scope, RenderScope::GetColorFormat(), Extents);
	}

	inline std::shared_ptr<GVkImage> DepthRenderTargetFlight(std::shared_ptr<RenderScope> Scope, VkExtent2D Extents)
	{
		return RenderTargetFlight(Scope, RenderScope::GetDepthFormat(), Extents);
	}

	inline std::shared_ptr<GVkImage> ReadOnlySampler2D(std::shared_ptr<RenderScope> Scope, VkExtent2D Extents, void* Pixels, size_t PixelsSize)
	{
		return Image(Scope, VK_FORMAT_R8G8B8A8_UNORM, { Extents.width, Extents.height, 1 }, EImageFlags::Sampler | EImageFlags::AllocateMipMaps | EImageFlags::TransferTarget, Pixels, PixelsSize);
	}

	inline std::shared_ptr<GVkImage> SolidColor(std::shared_ptr<RenderScope> Scope, uint32_t Color)
	{
		return ReadOnlySampler2D(Scope, VkExtent2D{ 1, 1 }, &Color, sizeof(uint32_t));
	}
};