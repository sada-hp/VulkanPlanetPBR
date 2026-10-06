#pragma once
#include "VulkanAPI/vertex.hpp"
#include "VulkanAPI/scope.hpp"
#include "VulkanAPI/mesh.hpp"

namespace GVkMeshFactory
{
	inline std::shared_ptr<IMesh> Mesh(std::shared_ptr<RenderScope> Scope, const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices)
	{
		return std::shared_ptr<IMesh>(new GVkMesh<MeshVertex>(Scope, vertices, indices));
	}

	inline std::shared_ptr<IMesh> Mesh(std::shared_ptr<RenderScope> Scope, const std::vector<MeshVertex>& vertices)
	{
		return std::shared_ptr<IMesh>(new GVkMesh<MeshVertex>(Scope, vertices));
	}

	inline std::shared_ptr<IMesh> Plane(std::shared_ptr<RenderScope> Scope, uint32_t width, uint32_t height, uint32_t slices = 1u, uint32_t stacks = 1u)
	{
		std::vector<MeshVertex> vertices; std::vector<uint32_t> indices;

		float dx = static_cast<float>(width) / static_cast<float>(slices);
		float dy = static_cast<float>(height) / static_cast<float>(stacks);

		for (uint32_t x = 0; x <= slices; x++)
		{
			for (uint32_t y = 0; y <= stacks; y++)
			{
				MeshVertex vertex{};
				vertex.position = { x * dx, y * dy, 0.0 };
				vertices.push_back(vertex);
			}
		}

		uint32_t triangles = slices * stacks * 2;
		uint32_t rowspan = slices + 1;
		uint32_t colspan = stacks + 1;

		indices.reserve(triangles * 3);
		for (uint32_t t = 0; t < triangles; t++)
		{
			uint32_t i0, i1, i2;

			if (t % 2 == 0)
			{
				i0 = t;
				i1 = t + 1;
				i2 = t + rowspan;
			}
			else
			{
				i0 = t + rowspan;
				i1 = t + rowspan - 1;
				i2 = t;
			}

			indices.push_back(i0);
			indices.push_back(i1);
			indices.push_back(i2);
		}

		return Mesh(Scope, vertices, indices);
	}
};