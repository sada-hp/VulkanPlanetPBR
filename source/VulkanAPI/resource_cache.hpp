#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <string>

class IShader;
class IPipelineDescriptor;

struct GHash
{
	static size_t Hash(const std::string& s, size_t seed = 42)
	{
		size_t h = seed;
		for (char ch : s)
			h = (h ^ ch) * 16777619;

		return h;
	}
};

class GVkResourceCache
{
	struct GVkShaderCache
	{
		std::vector<uint32_t> code = {};
		VkShaderModule module = VK_NULL_HANDLE;
	};

	friend class RenderScope;

private:
	static constexpr uint32_t magic = 0x8340452;
	const std::string shader_cache_dir;

private:
	std::unordered_map<size_t, VkPipelineCache> m_pipelineCache = {};
	std::unordered_map<size_t, GVkShaderCache> m_shaderCache = {};
	VkDevice m_vkDevice = VK_NULL_HANDLE;

private:
	void _flush();

	void _compile(size_t, GVkShaderCache&, bool bWrite = true);
	bool _restore_shader(size_t hash);

public:
	GVkResourceCache(VkDevice);
	~GVkResourceCache();

	VkShaderModule  Get(const IShader&);
	VkPipelineCache Get(const IPipelineDescriptor&);
};