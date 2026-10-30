#pragma once
#include "VulkanAPI/command_buffer.hpp"

namespace GVkBufferFactory
{
	inline std::shared_ptr<GVkBuffer> Buffer(std::shared_ptr<RenderScope> Scope, EBufferFlags Flags, size_t Size, void* Data = nullptr)
	{
		auto _buffer = std::make_shared<GVkBuffer>(Scope, Size, Flags);

		if (Data)
		{
			std::shared_ptr<GVkCommandBuffer> _cmd = std::make_shared<GVkCommandBuffer>(Scope, VK_QUEUE_GRAPHICS_BIT);
			auto view = GVkBuffer::ToView(_buffer);

			if (_buffer->IsInFlight())
			{
				Scope->ExecuteForAllFrames([_cmd, view, Data]()
				{
					_cmd->UpdateBuffer(view, (char*)Data);
				});
			}
			else
			{
				_cmd->UpdateBuffer(view, (char*)Data);
			}
		}

		return _buffer;
	}

	inline std::shared_ptr<GVkBuffer> IndexBuffer(std::shared_ptr<RenderScope> Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Index, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> IndexBufferFlight(std::shared_ptr<RenderScope> Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Index | EBufferFlags::InFlight, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> VertexBuffer(std::shared_ptr<RenderScope> Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Vertex, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> VertexBufferFlight(std::shared_ptr<RenderScope> Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Vertex | EBufferFlags::InFlight, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> UniformBuffer(std::shared_ptr<RenderScope> Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Uniform, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> UniformBufferFlight(std::shared_ptr<RenderScope> Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Uniform | EBufferFlags::InFlight, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> StorageBuffer(std::shared_ptr<RenderScope> Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Storage, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> StorageBufferFlight(std::shared_ptr<RenderScope> Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Storage | EBufferFlags::InFlight, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> IndirectBuffer(std::shared_ptr<RenderScope> Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Conditional | EBufferFlags::Storage | EBufferFlags::Indirect, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> IndirectBufferFlight(std::shared_ptr<RenderScope> Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Conditional | EBufferFlags::Storage | EBufferFlags::Indirect | EBufferFlags::InFlight, Size, Data);
	}
	
	inline std::shared_ptr<GVkBuffer> StagingBuffer(std::shared_ptr<RenderScope> Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Mapped | EBufferFlags::TransferSrc, Size, Data);
	}
};