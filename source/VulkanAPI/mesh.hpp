#pragma once
#include "Factories/VkBufferFactory.hpp"

class IMesh
{
protected:
	std::shared_ptr<GVkBufferView> m_VertexBufferView = VK_NULL_HANDLE;
	std::shared_ptr<GVkBuffer> m_VertexBuffer = VK_NULL_HANDLE;
	uint32_t m_VertexCount = 0;

	std::shared_ptr<GVkBufferView> m_IndexBufferView = VK_NULL_HANDLE;
	std::shared_ptr<GVkBuffer> m_IndexBuffer = VK_NULL_HANDLE;
	VkIndexType m_IndexType = VK_INDEX_TYPE_MAX_ENUM;
	uint32_t m_IndexCount = 0;

	glm::vec3 m_BoxMin = glm::vec3(0.0);
	glm::vec3 m_BoxMax = glm::vec3(0.0);

	std::shared_ptr<RenderScope> Scope = VK_NULL_HANDLE;

protected:
	void build_views()
	{
		if (m_VertexBuffer)
			m_VertexBufferView = GVkBuffer::ToView(m_VertexBuffer);

		if (m_IndexBuffer)
			m_IndexBufferView = GVkBuffer::ToView(m_IndexBuffer);
	}

public:
	IMesh(std::shared_ptr<RenderScope> InScope)
		: Scope(InScope)
	{
	}

	virtual ~IMesh()
	{
	}

	std::shared_ptr<GVkBufferView> VertexBuffer() const { return m_VertexBufferView; }
	uint32_t VertexCount() const { return m_VertexCount; }

	std::shared_ptr<GVkBufferView> IndexBuffer() const { return m_IndexBufferView; }
	VkIndexType IndexType() const { return m_IndexType; }
	uint32_t IndexCount() const { return m_IndexCount; }

	std::shared_ptr<RenderScope> GetScope() const
	{
		return Scope;
	}

	VkDrawIndexedIndirectCommand GetDrawCommand()
	{
		VkDrawIndexedIndirectCommand cmd{};
		if (m_IndexCount > 0)
		{
			cmd = {
				m_IndexCount, 1, 0, 0, 0
			};
		}
		else
		{
			cmd = {
				m_VertexCount, 1, 0, 0, 0
			};
		}

		return cmd;
	}

	const glm::vec3& GetMinPoint() const
	{
		return m_BoxMin;
	}

	const glm::vec3& GetMaxPoint() const
	{
		return m_BoxMax;
	}
};

template<typename VertexType> 
class GVkMesh : public IMesh
{
	void find_aabb(const std::vector<VertexType>& vertices)
	{
		m_BoxMin = glm::vec3(INFINITY);
		m_BoxMax = glm::vec3(-INFINITY);

		for (auto& vertex : vertices)
		{
			glm::vec3 position = vertex.position;
			m_BoxMin = glm::min(m_BoxMin, position);
			m_BoxMax = glm::max(m_BoxMax, position);
		}
	}

public:
	GVkMesh(std::shared_ptr<RenderScope> InScope, const std::vector<VertexType>& vertices, const std::vector<uint32_t>& indices, const glm::vec3& aabb_min = glm::vec3(-INFINITY), const glm::vec3& aabb_max = glm::vec3(INFINITY))
		: IMesh(InScope)
	{
		m_IndexType = VK_INDEX_TYPE_UINT32;
		m_VertexCount = vertices.size();
		m_IndexCount = indices.size();

		m_VertexBuffer = GVkBufferFactory::VertexBuffer(Scope, sizeof(VertexType) * m_VertexCount, (VertexType*)vertices.data());
		m_IndexBuffer = GVkBufferFactory::IndexBuffer(Scope, sizeof(uint32_t) * m_IndexCount, (uint32_t*)indices.data());
		build_views();

		m_BoxMin = aabb_min;
		m_BoxMax = aabb_max;

		if (m_BoxMin == glm::vec3(-INFINITY) || m_BoxMax == glm::vec3(INFINITY))
			find_aabb(vertices);
	}

	GVkMesh(std::shared_ptr<RenderScope> InScope, const std::vector<VertexType>& vertices, const std::vector<uint16_t>& indices, const glm::vec3& aabb_min = glm::vec3(-INFINITY), const glm::vec3& aabb_max = glm::vec3(INFINITY))
		: IMesh(InScope)
	{
		m_IndexType = VK_INDEX_TYPE_UINT16;
		m_VertexCount = vertices.size();
		m_IndexCount = indices.size();

		m_VertexBuffer = GVkBufferFactory::VertexBuffer(Scope, sizeof(VertexType) * m_VertexCount, (VertexType*)vertices.data());
		m_IndexBuffer = GVkBufferFactory::IndexBuffer(Scope, sizeof(uint16_t) * m_IndexCount, (uint16_t*)indices.data());
		build_views();

		m_BoxMin = aabb_min;
		m_BoxMax = aabb_max;

		if (m_BoxMin == glm::vec3(-INFINITY) || m_BoxMax == glm::vec3(INFINITY))
			find_aabb(vertices);
	}

	GVkMesh(std::shared_ptr<RenderScope> InScope, const std::vector<VertexType>& vertices, const glm::vec3& aabb_min = glm::vec3(-INFINITY), const glm::vec3& aabb_max = glm::vec3(INFINITY))
		: IMesh(InScope)
	{
		m_VertexCount = vertices.size();
		m_VertexBuffer = GVkBufferFactory::VertexBuffer(Scope, sizeof(VertexType) * m_VertexCount, (VertexType*)vertices.data());
		build_views();

		m_BoxMin = aabb_min;
		m_BoxMax = aabb_max;

		if (m_BoxMin == glm::vec3(-INFINITY) || m_BoxMax == glm::vec3(INFINITY))
			find_aabb(vertices);
	}

	virtual ~GVkMesh()
	{

	}
};