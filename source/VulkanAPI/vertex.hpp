#pragma once
#include <vulkan/vulkan.h>
#include "glm/gtx/hash.hpp"
#include "glm/glm.hpp"

struct MeshVertex
{
public:
	glm::vec3 position = glm::vec3(0.0);
	uint32_t  submesh  = glm::uint(0u);
	glm::vec3 normal   = glm::vec3(0.0);
	glm::vec3 tangent  = glm::vec3(0.0);
	glm::vec2 uv       = glm::vec2(0.0);

public:
	bool operator==(const MeshVertex& other) const
	{
		return position == other.position
			&& normal == other.normal
			&& tangent == other.tangent
			&& uv == other.uv
			&& submesh == other.submesh;
	}

public:
	static const std::vector<VkVertexInputBindingDescription> getBindingDescriptions()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(MeshVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return std::vector{ bindingDescription };
	}

	static const std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(MeshVertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[1].offset = offsetof(MeshVertex, submesh);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(MeshVertex, normal);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(MeshVertex, tangent);

		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(MeshVertex, uv);

		return attributeDescriptions;
	}
};

struct TerrainVertex
{
public:
	glm::vec4 position;
	glm::vec4 uv;

public:
	bool operator==(const TerrainVertex& other) const
	{
		return position == other.position && uv == other.uv;
	}

public:
	static const VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(TerrainVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static const std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(TerrainVertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(TerrainVertex, uv);

		return attributeDescriptions;
	}
};

template<>
struct std::hash<MeshVertex>
{
	size_t operator()(MeshVertex const& vertex) const
	{
		return ((std::hash<glm::vec3>()(vertex.position)
			^ (std::hash<glm::vec3>()(vertex.normal))
			^ (std::hash<glm::vec3>()(vertex.tangent))
			^ (std::hash<glm::vec2>()(vertex.uv))
			^ (std::hash<uint32_t>()(vertex.submesh))) >> 1);
	}
};