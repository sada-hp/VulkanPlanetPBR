#pragma once
#include "Shaders/ShaderLibrary.hpp"

class IShader
{
public:
	virtual VkShaderStageFlags GetStage() const = 0;
	virtual ShaderCodeType& GetCode() const = 0;
	virtual std::string HashString() const = 0;
	virtual bool IsValid() const = 0;
};

template<VkShaderStageFlags Stage>
class GVkShader : public IShader
{
	std::string m_Code = "";
	std::string m_Preprocessor = "";
	mutable std::string m_resultShader = "";

public:

	VkShaderStageFlags GetStage() const override 
	{ 
		return Stage; 
	}

	GVkShader& AppendCode(ShaderCodeType& code)
	{
		m_Code += code + "\n";
		m_resultShader = m_Preprocessor + m_Code;

		return *this;
	}

	GVkShader& AddDefine(ShaderCodeType& value)
	{
		m_Preprocessor += std::string("\n #define ") + value + std::string(" \n");
		m_resultShader = m_Preprocessor + m_Code;

		return *this;
	}

	ShaderCodeType& GetCode() const override
	{
		return m_resultShader;
	}

	std::string HashString() const override
	{
		return m_resultShader;
	}

	virtual bool IsValid() const override
	{
		return !m_resultShader.empty();
	}
};