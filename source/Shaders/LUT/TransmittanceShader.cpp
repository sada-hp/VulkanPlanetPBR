#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderPrecomputeLUT::TransmittancePS)
(
    layout(location = 0) in vec2 UV;
    layout(location = 0) out vec4 Color;

    float GetOpticalDepth(float H, float R, float Mu)
    {
        if (Mu < -sqrt(1.0 - (Rg / R) * (Rg / R)))
            return 1e9;

        float Depth = 0.0;
        float Dx = DistanceToAtmosphere(R, Mu) / float(TRANSMITTANCE_SAMPLES);
        float Xi = 0.0;
        float Yi = GetAirDensity(R, H);

        for (int i = 1; i < TRANSMITTANCE_SAMPLES; i++)
        {
            float Xj = float(i) * Dx;
            float Yj = exp(-(sqrt(R * R + Xj * Xj + 2.0 * Xj * R * Mu) - Rg) / H);
            Depth += (Yi + Yj) * 0.5 * Dx;
            Xi = Xj;
            Yi = Yj;
        }

        return Depth;
    }

	void main()
	{
        float R, Mu;
        GetTransmittanceRMu(UV, R, Mu);

        vec3 Depth = BetaR * GetOpticalDepth(HR, R, Mu) + BetaMEx * GetOpticalDepth(HM, R, Mu);
        Color = vec4(exp(-Depth), 0.0);
	}
);