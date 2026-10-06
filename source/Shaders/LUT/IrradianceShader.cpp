#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderPrecomputeLUT::IrradiancePS)
(
	layout(location = 0) in vec2 UV;
	layout(location = 0) out vec4 Irradiance;

	layout(binding = 0) uniform sampler2D TransmittanceLUT;

	void main()
	{
		float R, Mu;
		GetIrradienceRMu(UV, R, Mu);
		Irradiance = vec4(GetTransmittance(TransmittanceLUT, R, Mu) * max(Mu, 0.0), 0.0);
	}
);