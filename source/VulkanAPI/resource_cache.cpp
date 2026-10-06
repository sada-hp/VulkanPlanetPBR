#include "resource_cache.hpp"
#include "shaderc/shaderc.hpp"

#include "shader.hpp"
#include "pipeline.hpp" 

#include <filesystem>
#include <fstream>

GVkResourceCache::GVkResourceCache(VkDevice device)
	: shader_cache_dir(std::filesystem::current_path().string() + "\\cache\\shaders\\")
	, m_vkDevice(device)
{
	_restore();
}

GVkResourceCache::~GVkResourceCache()
{
	_flush();
}

void GVkResourceCache::_restore()
{
	if (std::filesystem::exists(shader_cache_dir))
	{
		for (auto file : std::filesystem::directory_iterator(shader_cache_dir))
		{
			size_t hash = std::stoull(file.path().filename());
			std::string path = file.path().string();

			std::ifstream inFile(path, std::ios::binary);
			if (inFile.is_open())
			{
				size_t bufferSize = file.file_size();
				std::vector<char> buffer(bufferSize);

				if (inFile.read(buffer.data(), buffer.size()))
				{
					std::vector<uint32_t> code = std::vector(reinterpret_cast<uint32_t*>(buffer.data()), reinterpret_cast<uint32_t*>(buffer.data() + buffer.size()));

					if (code.back() == magic)
					{
						code.erase(std::prev(code.end()));

						m_shaderCache[hash].code = std::move(code);
						_compile(hash, m_shaderCache[hash], false);
					}
				}

				inFile.close();
			}
		}
	}
	else
	{
		std::filesystem::create_directories(shader_cache_dir);
	}
}

void GVkResourceCache::_compile(size_t hash, GVkShaderCache& cache, bool bWrite)
{
	if (cache.code.empty())
		return;

	VkShaderModuleCreateInfo shaderCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	shaderCreateInfo.codeSize = cache.code.size() * sizeof(uint32_t);
	shaderCreateInfo.pCode = cache.code.data();

	if (vkCreateShaderModule(m_vkDevice, &shaderCreateInfo, VK_NULL_HANDLE, &cache.module) == VK_SUCCESS)
	{
		if (bWrite)
		{
			std::string fileName = shader_cache_dir + std::to_string(hash);

			std::ofstream outFile(fileName, std::ios::binary);

			if (outFile.is_open())
			{
				outFile.write(reinterpret_cast<const char*>(cache.code.data()), sizeof(uint32_t) * cache.code.size());
				outFile.write(reinterpret_cast<const char*>(&magic), sizeof(uint32_t));
			}

			outFile.close();
		}
	}
	else
	{
		cache.module = VK_NULL_HANDLE;
		cache.code.clear();
	}
}

void GVkResourceCache::_flush()
{
	for (auto& [hash, cache] : m_shaderCache)
		vkDestroyShaderModule(m_vkDevice, cache.module, VK_NULL_HANDLE);

	m_shaderCache.clear();

	for (auto& [hash, cache] : m_pipelineCache)
		vkDestroyPipelineCache(m_vkDevice, cache, VK_NULL_HANDLE);

	m_pipelineCache.clear();
}

VkShaderModule GVkResourceCache::Get(const IShader& Shader)
{
	size_t Hash = GHash::Hash(Shader.HashString());

	if (!m_shaderCache.contains(Hash) || m_shaderCache[Hash].module == VK_NULL_HANDLE)
	{
		shaderc_shader_kind kind;
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		switch (Shader.GetStage())
		{
			case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: kind = shaderc_shader_kind::shaderc_glsl_default_tess_control_shader; break;
			case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: kind = shaderc_shader_kind::shaderc_glsl_tess_evaluation_shader; break;
			case VK_SHADER_STAGE_GEOMETRY_BIT: kind = shaderc_shader_kind::shaderc_geometry_shader; break;
			case VK_SHADER_STAGE_FRAGMENT_BIT: kind = shaderc_shader_kind::shaderc_fragment_shader; break;
			case VK_SHADER_STAGE_COMPUTE_BIT: kind = shaderc_shader_kind::shaderc_compute_shader; break;
			default: kind = shaderc_shader_kind::shaderc_glsl_vertex_shader; break;
		};

		std::string Code = "#version 460 \n" + Shader.GetCode();
		options.SetOptimizationLevel(shaderc_optimization_level_size);
		shaderc::PreprocessedSourceCompilationResult PreprocessedGLSL = compiler.PreprocessGlsl(Code.c_str(), kind, "shader", options);

		options.SetTargetSpirv(shaderc_spirv_version_1_6);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(PreprocessedGLSL.begin(), std::distance(PreprocessedGLSL.begin(), PreprocessedGLSL.end()), kind, "shader", "main", options);

		if (module.GetCompilationStatus() == shaderc_compilation_status_success)
		{
			m_shaderCache[Hash].code = std::vector(module.begin(), module.end());
			_compile(Hash, m_shaderCache[Hash]);
		}
		else
		{
			std::cerr << module.GetErrorMessage() << std::endl;
			return VK_NULL_HANDLE;
		}
	}
	
	return m_shaderCache[Hash].module;
}

VkPipelineCache GVkResourceCache::Get(const IPipelineDescriptor& Pipeline)
{
	size_t Hash = GHash::Hash(Pipeline.HashString());

	if (!m_pipelineCache.contains(Hash))
	{
		VkPipelineCacheCreateInfo cacheCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
		vkCreatePipelineCache(m_vkDevice, &cacheCreateInfo, VK_NULL_HANDLE, &m_pipelineCache[Hash]);
	}

	return m_pipelineCache[Hash];
}