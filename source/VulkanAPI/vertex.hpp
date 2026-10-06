#pragma once
#include <vulkan/vulkan.h>
#include "glm/gtx/hash.hpp"
#include "glm/glm.hpp"

struct MeshVertex
{
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
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(MeshVertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(MeshVertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(MeshVertex, tangent);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(MeshVertex, uv);

		return attributeDescriptions;
	}

	bool operator==(const MeshVertex& other) const
	{
		return position == other.position
			&& normal == other.normal
			&& tangent == other.tangent
			&& uv == other.uv
			&& submesh == other.submesh;
	}

	uint32_t submesh = 0;
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec2 uv;
};

struct TerrainVertex
{
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

	bool operator==(const TerrainVertex& other) const
	{
		return position == other.position && uv == other.uv;
	}

	glm::vec4 position;
	glm::vec4 uv;
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