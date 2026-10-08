#pragma once
#include <string>

#define Stringify(...) #__VA_ARGS__
#define ShaderCodeType const std::string
#define ShaderCodeDefinition(x, ...) ShaderCodeType x = Stringify

class GShaderUtils
{
public:
	static ShaderCodeType UBOCommon;
	static ShaderCodeType UtilsCommon;
	static ShaderCodeType NoiseCommon;
	static ShaderCodeType LightingCommon;
};

class GShaderPrecomputeLUT
{
public:
	static ShaderCodeType LUTCommon;
	static ShaderCodeType IrradiancePS;
	static ShaderCodeType ScatteringGS;
	static ShaderCodeType TransmittancePS;
	static ShaderCodeType SingleScatteringPS;
	static ShaderCodeType MulitScatteringAddPS;
	static ShaderCodeType IrradianceMultiStepPS;
	static ShaderCodeType MulitScatteringEvaluatePS;
};

class GShaderNoise
{
public:
	static ShaderCodeType PerlinWorleyPS;
	static ShaderCodeType WorleyPS;
	static ShaderCodeType NoiseGS;
};

class GShaders
{
public:
	//Fullscreen
	static ShaderCodeType FullscreenVS;
	static ShaderCodeType FullscreenLayeredVS;
	// Sky
	static ShaderCodeType AtmospherePS;
	static ShaderCodeType CloudsPS;
	// Mesh
	static ShaderCodeType MeshVS;
	static ShaderCodeType MeshPS;
	static ShaderCodeType MeshDefaultPS;
	// Post process
	static ShaderCodeType TonemapPS;
};