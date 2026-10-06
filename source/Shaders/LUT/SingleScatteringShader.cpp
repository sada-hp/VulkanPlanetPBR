#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderPrecomputeLUT::ScatteringGS)
(
	layout(triangles) in;
	layout(triangle_strip, max_vertices = 3) out;

	layout(location = 0) in vec3 inUVW[];
    layout(location = 0) out vec3 outUVW;

	void main()
	{
		for (int vertex = 0; vertex < 3; vertex++)
		{
			gl_Position = gl_in[vertex].gl_Position;
			gl_Layer = int(inUVW[vertex].z);
            outUVW = inUVW[vertex];
			EmitVertex();
		}

		EndPrimitive();
	}
);

ShaderCodeDefinition(GShaderPrecomputeLUT::SingleScatteringPS)
(
	layout(location = 0) in vec3 UVW;

	layout(location = 0) out vec4 DeltaSR;
	layout(location = 1) out vec4 DeltaSM;
	layout(location = 2) out vec4 Scattering;

	layout(binding = 0) uniform sampler2D TransmittanceLUT;

    void RayMieIntegrand(float R, float Mu, float MuS, float Nu, float t, out vec3 Ray, out vec3 Mie)
    {
        Ray = vec3(0.0);
        Mie = vec3(0.0);

        float Ri = sqrt(R * R + t * t + 2.0 * R * Mu * t);
        float MuSi = (Nu * t + MuS * R) / Ri;
        Ri = clamp(Ri, Rg, Rt);

        if (MuSi >= -sqrt(1.0 - Rg * Rg / (Ri * Ri)))
        {
            vec3 T = GetTransmittance(TransmittanceLUT, R, Mu, t) * GetTransmittance(TransmittanceLUT, Ri, MuSi);
            Ray = GetAirDensity(Ri, HR) * T;
            Mie = GetAirDensity(Ri, HM) * T;
        }
    }

    void ComputeDeltaSRDeltaSM(float R, float Mu, float MuS, float Nu, out vec3 Ray, out vec3 Mie)
    {
        Ray = vec3(0.0);
        Mie = vec3(0.0);

        float Dx = DistanceToAtmosphere(R, Mu) / float(INSCATTERING_SAMPLES);

        vec3 Rayi, Miei;
        RayMieIntegrand(R, Mu, MuS, Nu, 0.0, Rayi, Miei);

        for (int i = 1; i <= INSCATTERING_SAMPLES; i++)
        {
            float Xi = float(i) * Dx;

            vec3 Rayj, Miej;
            RayMieIntegrand(R, Mu, MuS, Nu, Xi, Rayj, Miej);

            Ray += (Rayi + Rayj) / 2.0 * Dx;
            Mie += (Miei + Miej) / 2.0 * Dx;

            Rayi = Rayj;
            Miei = Miej;
        }

        Ray *= BetaR;
        Mie *= BetaMSca;
    }

	void main()
	{
        float R, CosViewZenith, CosSunZenith, CosViewun;
        UVWToWorldInscatter(UVW, R, CosViewZenith, CosSunZenith, CosViewun);

        DeltaSR = vec4(0.0); DeltaSM = vec4(0.0); Scattering = vec4(0.0);
        ComputeDeltaSRDeltaSM(R, CosViewZenith, CosSunZenith, CosViewun, DeltaSR.rgb, DeltaSM.rgb);
        Scattering = vec4(DeltaSR.rgb, DeltaSM.r);
	}
);
