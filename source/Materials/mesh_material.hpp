#pragma once
#include "IMaterial.hpp"

class GMeshMaterial : public IMaterial
{
public:
	GMeshMaterial(std::shared_ptr<RenderScope> Scope, VkCullModeFlagBits CullMode, VkPrimitiveTopology Topology);

	static std::shared_ptr<GMeshMaterial> Create(std::shared_ptr<RenderScope> Scope, VkCullModeFlagBits CullMode = VK_CULL_MODE_BACK_BIT, VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
	{
		return std::make_shared<GMeshMaterial>(Scope, CullMode, Topology);
	}
};