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

	inline std::vector<uint32_t> GenerateQuadIndices(uint32_t slices, uint32_t stacks, uint32_t base_index = 0)
	{
		std::vector<uint32_t> indices = {};

		uint32_t triangles = slices * stacks * 2;
		indices.reserve(triangles * 3);

		for (uint32_t y = 0; y < stacks; y++)
		{
			for (uint32_t x = 0; x < slices; x++)
			{
				uint32_t base = base_index + x + y * (slices + 1);
				uint32_t next_col = base + 1;
				uint32_t next_row = base + slices + 1;

				indices.push_back(base);
				indices.push_back(next_row);
				indices.push_back(next_col);

				indices.push_back(next_col);
				indices.push_back(next_row);
				indices.push_back(next_row + 1);
			}
		}

		return indices;
	}

	inline void CalculateNormals(std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices)
	{
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			MeshVertex& A = vertices[indices[i]];
			MeshVertex& B = vertices[indices[i + 1]];
			MeshVertex& C = vertices[indices[i + 2]];
			glm::vec3  contributingNormal = glm::cross(glm::vec3(B.position) - glm::vec3(A.position), glm::vec3(C.position) - glm::vec3(A.position));

			vertices[indices[i]].normal += contributingNormal;
			vertices[indices[i + 1]].normal += contributingNormal;
			vertices[indices[i + 2]].normal += contributingNormal;
		}

		for (size_t i = 0; i < vertices.size(); i++)
			vertices[i].normal = glm::normalize(vertices[i].normal);
	}

	inline std::shared_ptr<IMesh> Plane(std::shared_ptr<RenderScope> Scope, float width, float height, uint32_t slices = 1u, uint32_t stacks = 1u)
	{
		std::vector<MeshVertex> vertices = {};

		float uv_dx = 1.f / static_cast<float>(slices);
		float uv_dy = 1.f / static_cast<float>(stacks);

		float pos_dx = width * uv_dx;
		float pos_dy = height * uv_dy;

		vertices.reserve((slices + 1) * (stacks + 1));
		for (uint32_t y = 0; y <= stacks; y++)
		{
			for (uint32_t x = 0; x <= slices; x++)
			{
				MeshVertex vertex{};
				vertex.position = { x * pos_dx - width / 2.0, 0.0, y * pos_dy - height / 2.0 };
				vertex.uv = { x * uv_dx, y * uv_dy };
				vertices.push_back(vertex);
			}
		}
		
		std::vector<uint32_t> indices = GenerateQuadIndices(slices, stacks);
		CalculateNormals(vertices, indices);

		return Mesh(Scope, vertices, indices);
	}

	inline std::shared_ptr<IMesh> Cube(std::shared_ptr<RenderScope> Scope, float width, float height, float depth, uint32_t slices = 1u, uint32_t stacks = 1u, bool separate_sumbeshes_by_face = false)
	{
		std::vector<MeshVertex> vertices = {}; std::vector<uint32_t> indices = {};

		float uv_dx = 1.f / static_cast<float>(slices);
		float uv_dy = 1.f / static_cast<float>(stacks);

		float pos_dx = width * uv_dx;
		float pos_dy = height * uv_dy;

		int face = 0;
		auto generateFace = [&](float p, float y, float r)
		{
			glm::quat rot = glm::vec3(glm::radians(p), glm::radians(y), glm::radians(r));

			std::vector<uint32_t> face_indices = GenerateQuadIndices(slices, stacks, vertices.size());
			indices.insert(indices.end(), face_indices.begin(), face_indices.end());

			for (uint32_t y = 0; y <= stacks; y++)
			{
				for (uint32_t x = 0; x <= slices; x++)
				{
					MeshVertex vertex{};
					vertex.position = rot * glm::vec3{ x * pos_dx - width / 2.0, depth / 2.0, y * pos_dy - height / 2.0 };
					vertex.uv = { x * uv_dx, y * uv_dy };

					if (separate_sumbeshes_by_face)
						vertex.submesh = face;

					vertices.push_back(vertex);
				}
			}

			face++;
		};

		// top
		generateFace(0.0, 0.0, 0.0);
		// bottom
		generateFace(180.0, 0.0, 0.0);
		// front
		generateFace(90.0, 0.0, 0.0);
		// back
		generateFace(-90.0, 0.0, 0.0);
		// side
		generateFace(0.0, 0.0, 90.0);
		// other side
		generateFace(0.0, 0.0, -90.0);

		CalculateNormals(vertices, indices);
		return Mesh(Scope, vertices, indices);
	}
};