#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderPrecomputeLUT::LUTCommon)
(
    const int INSCATTERING_SPHERE_SAMPLES = 32;
    const int TRANSMITTANCE_SAMPLES = 500;
    const int INSCATTERING_SAMPLES = 500;
    const int IRRADIANCE_SAMPLES = 32;

    float DistanceToAtmosphere(float R, float Mu)
    {
        float Dout = -R * Mu + sqrt(R * R * (Mu * Mu - 1.0) + Rt * Rt);
        float Delta2 = R * R * (Mu * Mu - 1.0) + Rg * Rg;

        if (Delta2 >= 0.0)
        {
            float Din = -R * Mu - sqrt(Delta2);

            if (Din >= 0.0)
            {
                Dout = min(Dout, Din);
            }
        }

        return Dout;
    }

    float GetAirDensity(float R, float H)
    {
        return exp(-(R - Rg) / H);
    }

    void GetTransmittanceRMu(vec2 UV, out float R, out float Mu)
    {
        R = Rg + (UV.y * UV.y) * (Rt - Rg);
        Mu = -0.15 + tan(1.5 * UV.x) / tan(1.5) * (1.0 + 0.15);
    }

    void GetIrradienceRMu(vec2 UV, out float R, out float Mu)
    {
        R = Rg + UV.y * (Rt - Rg);
        Mu = -0.2 + UV.x * (1.0 + 0.2);
    }

    void UVWToWorldInscatter(vec3 UVW, out float R, out float CosViewZenith, out float CosSunZenith, out float CosViewun)
    {
        const float dHdH = Rt * Rt - Rg * Rg;

        R = UVW.z / (DIM_R - 1.0);
        R = sqrt(Rg * Rg + R * R * dHdH) + (UVW.z == 0 ? 0.01 : ((UVW.z == DIM_R - 1) ? -0.001 : 0.0));

        float x = gl_FragCoord.x - 0.5;
        float y = gl_FragCoord.y - 0.5;

        if (UVW.y < 0.5)
        {
            float Dmax = sqrt(R * R - Rg * Rg);
            float D = 1.0 - y / (float(DIM_MU) * 0.5 - 1.0);

            D = clamp(R - Rg, D * Dmax, Dmax * 0.999);
            CosViewZenith = (Rg * Rg - R * R - D * D) / (2.0 * R * D);
            CosViewZenith = min(CosViewZenith, -sqrt(1.0 - (Rg / R) * (Rg / R)) - 0.001);
        }
        else
        {
            float Dmax = sqrt(R * R - Rg * Rg) + sqrt(dHdH);
            float D = (y - float(DIM_MU) * 0.5) / (float(DIM_MU) * 0.5 - 1.0);

            D = clamp(Rt - R, D * Dmax, Dmax * 0.999);
            CosViewZenith = (Rt * Rt - R * R - D * D) / (2.0 * R * D);
        }

        CosSunZenith = mod(x, float(DIM_MU_S)) / (float(DIM_MU_S) - 1.0);
        CosSunZenith = tan((2.0 * CosSunZenith - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1);

        CosViewun = -1.0 + floor(x / float(DIM_MU_S)) / (float(DIM_NU) - 1.0) * 2.0;
    }
);