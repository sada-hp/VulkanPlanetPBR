#include "pch.hpp"
#include "mesh.hpp"
#include "Factories/VkBufferFactory.h"

VulkanMesh::VulkanMesh(const RenderScope& InScope, MeshVertex* vertices, size_t numVertices, uint32_t* indices, size_t numIndices)
	: Scope(&InScope)
{
	vertexBuffer = VkBufferFactory::VertexBuffer(InScope, sizeof(MeshVertex) * numVertices, vertices);
	indexBuffer = VkBufferFactory::IndexBuffer(InScope, sizeof(uint32_t) * numIndices, indices);

	verticesCount = numVertices;
	indicesCount = numIndices;
	indexType = VK_INDEX_TYPE_UINT32;
}

VulkanMesh::VulkanMesh(const RenderScope& InScope, MeshVertex* vertices, size_t numVertices, uint16_t* indices, size_t numIndices)
	: Scope(&InScope)
{
	vertexBuffer = VkBufferFactory::VertexBuffer(InScope, sizeof(MeshVertex) * numVertices, vertices);
	indexBuffer = VkBufferFactory::IndexBuffer(InScope, sizeof(uint16_t) * numIndices, indices);

	verticesCount = numVertices;
	indicesCount = numIndices;
	indexType = VK_INDEX_TYPE_UINT16;
}

VulkanMesh::VulkanMesh(const RenderScope& InScope, TerrainVertex* vertices, size_t numVertices, uint32_t* indices, size_t numIndices)
	: Scope(&InScope)
{
	vertexBuffer = VkBufferFactory::Buffer(InScope, EBufferFlags::Vertex | EBufferFlags::Storage | EBufferFlags::TransferSrc, sizeof(TerrainVertex) * numVertices, vertices);
	indexBuffer = VkBufferFactory::IndexBuffer(InScope, sizeof(uint32_t) * numIndices, indices);

	verticesCount = numVertices;
	indicesCount = numIndices;
	indexType = VK_INDEX_TYPE_UINT32;
}