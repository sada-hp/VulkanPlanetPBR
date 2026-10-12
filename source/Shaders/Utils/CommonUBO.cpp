#include "Shaders/ShaderLibrary.hpp"
#include "VulkanAPI/ubo.hpp"

ShaderCodeType GShaderUtils::UBOCommon = std::string("layout(set = 0, binding = 0) uniform UnfiormBuffer {") + std::string(StringifyMacro(UBO_contents)) + std::string("} UBO;");