#pragma once
#include "Factories/VkBufferFactory.hpp"

class IMesh
{
protected:
	std::shared_ptr<GVkBuffer> m_VertexBuffer = VK_NULL_HANDLE;
	uint32_t m_VertexCount = 0;

	std::shared_ptr<GVkBuffer> m_IndexBuffer = VK_NULL_HANDLE;
	VkIndexType m_IndexType = VK_INDEX_TYPE_MAX_ENUM;
	uint32_t m_IndexCount = 0;

	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;

public:
	IMesh(std::shared_ptr<RenderScope> InScope)
		: Scope(InScope)
	{
	}

	std::shared_ptr<GVkBuffer> VertexBuffer() const { return m_VertexBuffer; }
	uint32_t VertexCount() const { return m_VertexCount; }

	std::shared_ptr<GVkBuffer> IndexBuffer() const { return m_IndexBuffer; }
	VkIndexType IndexType() const { return m_IndexType; }
	uint32_t IndexCount() const { return m_IndexCount; }
};

template<typename VertexType> 
class GVkMesh : public IMesh
{
public:
	GVkMesh(std::shared_ptr<RenderScope> InScope, const std::vector<VertexType>& vertices, const std::vector<uint32_t>& indices)
		: IMesh(InScope)
	{
		m_IndexType = VK_INDEX_TYPE_UINT32;
		m_VertexCount = vertices.size();
		m_IndexCount = indices.size();

		m_VertexBuffer = GVkBufferFactory::VertexBuffer(Scope, sizeof(VertexType) * m_VertexCount, (VertexType*)vertices.data());
		m_IndexBuffer = GVkBufferFactory::IndexBuffer(Scope, sizeof(uint32_t) * m_IndexCount, (uint32_t*)indices.data());
	}

	GVkMesh(std::shared_ptr<RenderScope> InScope, const std::vector<VertexType>& vertices, const std::vector<uint16_t>& indices)
		: IMesh(InScope)
	{
		m_IndexType = VK_INDEX_TYPE_UINT16;
		m_VertexCount = vertices.size();
		m_IndexCount = indices.size();

		m_VertexBuffer = GVkBufferFactory::VertexBuffer(Scope, sizeof(VertexType) * m_VertexCount, (VertexType*)vertices.data());
		m_IndexBuffer = GVkBufferFactory::IndexBuffer(Scope, sizeof(uint16_t) * m_IndexCount, (uint16_t*)indices.data());
	}

	GVkMesh(std::shared_ptr<RenderScope> InScope, const std::vector<VertexType>& vertices)
		: IMesh(InScope)
	{
		m_VertexCount = vertices.size();
		m_VertexBuffer = GVkBufferFactory::VertexBuffer(Scope, sizeof(VertexType) * m_VertexCount, (VertexType*)vertices.data());
	}

	~GVkMesh()
	{

	}
};