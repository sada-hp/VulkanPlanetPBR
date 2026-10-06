#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderPrecomputeLUT::MulitScatteringAddPS)
(
    layout(location = 0) out vec4 Scattering;
    layout(location = 0) in vec3 UVW;

    layout(binding = 0) uniform sampler2D Transmittance;
    layout(binding = 1) uniform sampler3D DeltaJ;

    vec3 InscatteringIntegrand(float R, float Mu, float MuS, float Nu, float t)
    {
        float Ri = sqrt(R * R + t * t + 2.0 * R * Mu * t);
        float Mui = (R * Mu + t) / Ri;
        float MuSi = (Nu * t + MuS * R) / Ri;
        return GetInscattering(DeltaJ, Ri, Mui, MuSi, Nu).rgb * GetTransmittance(Transmittance, R, Mu, t);
    }

    vec3 ComputeDeltaS(float R, float Mu, float MuS, float Nu)
    {
        vec3 DeltaS = vec3(0.0);

        float Dx = DistanceToAtmosphere(R, Mu) / float(INSCATTERING_SAMPLES);

        vec3 Si = InscatteringIntegrand(R, Mu, MuS, Nu, 0.0);

        for (int i = 1; i <= INSCATTERING_SAMPLES; i++)
        {
            vec3 Sj = InscatteringIntegrand(R, Mu, MuS, Nu, i * Dx);
            DeltaS += (Si + Sj) * 0.5 * Dx;
            Si = Sj;
        }

        return DeltaS;
    }

	void main()
	{
        float R, CosViewZenith, CosSunZenith, CosViewun;
        UVWToWorldInscatter(UVW, R, CosViewZenith, CosSunZenith, CosViewun);
        Scattering = vec4(ComputeDeltaS(R, CosViewZenith, CosSunZenith, CosViewun), 0.0);
	}
);