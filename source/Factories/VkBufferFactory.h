#pragma once
#include "Vulkan/buffer.hpp"

namespace VkBufferFactory
{
	inline std::shared_ptr<GVkBuffer> Buffer(const RenderScope& Scope, EBufferFlags Flags, size_t Size, void* Data = nullptr)
	{
		auto _buffer = std::make_shared<GVkBuffer>(Scope, Size, Flags);

		if (Data)
			_buffer->Update(Data, Size);

		return _buffer;
	}

	inline std::shared_ptr<GVkBuffer> IndexBuffer(const RenderScope& Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Index, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> IndexBufferFlight(const RenderScope& Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Index | EBufferFlags::InFlight, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> VertexBuffer(const RenderScope& Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Vertex, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> VertexBufferFlight(const RenderScope& Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Vertex | EBufferFlags::InFlight, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> UniformBuffer(const RenderScope& Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Uniform, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> UniformBufferFlight(const RenderScope& Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Uniform | EBufferFlags::InFlight, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> StorageBuffer(const RenderScope& Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Storage, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> StorageBufferFlight(const RenderScope& Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Storage | EBufferFlags::InFlight, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> IndirectBuffer(const RenderScope& Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Indirect, Size, Data);
	}

	inline std::shared_ptr<GVkBuffer> IndirectBufferFlight(const RenderScope& Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Indirect | EBufferFlags::InFlight, Size, Data);
	}
	
	inline std::shared_ptr<GVkBuffer> StagingBuffer(const RenderScope& Scope, size_t Size, void* Data = nullptr)
	{
		return Buffer(Scope, EBufferFlags::Mapped | EBufferFlags::TransferSrc, Size, Data);
	}
};