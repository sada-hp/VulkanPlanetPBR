#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderPrecomputeLUT::IrradianceMultiStepPS)
(
    layout(location = 0) out vec4 Irradiance;
	layout(location = 0) in vec2 UV;

    layout(binding = 0) uniform sampler3D DeltaSR;
    layout(binding = 1) uniform sampler3D DeltaSM;

    vec3 ComputeDeltaEn(float R, float CosSunZenith)
    {
        const float dPhi = PI / float(IRRADIANCE_SAMPLES);
        const float dTheta = PI / float(IRRADIANCE_SAMPLES);

        vec3 Sun = vec3(sqrt(1.0 - CosSunZenith * CosSunZenith), 0.0, CosSunZenith);

        vec3 DeltaE = vec3(0.0);
        for (int iPhi = 0; iPhi < 2 * IRRADIANCE_SAMPLES; iPhi++)
        {
            float Phi = (iPhi + 0.5) * dPhi;

            for (int iTheta = 0; iTheta < IRRADIANCE_SAMPLES / 2; iTheta++)
            {
                float Theta = (iTheta + 0.5) * dTheta;
                vec3 W = vec3(cos(Phi) * sin(Theta), sin(Phi) * sin(Theta), cos(Theta));
                float dW = dTheta * dPhi * sin(Theta) * W.z;

                float CosViewun = dot(Sun, W);

                vec3 Ray = GetInscattering(DeltaSR, R, W.z, CosSunZenith, CosViewun).rgb;
                vec3 Mie = GetInscattering(DeltaSM, R, W.z, CosSunZenith, CosViewun).rgb;

                DeltaE += (Ray * RayleighPhase(CosViewun) + Mie * HGDPhase(CosViewun)) * dW;
            }
        }

        return DeltaE;
    }

	void main()
	{
        float R, Mu;
        GetIrradienceRMu(UV, R, Mu);
        Irradiance = vec4(ComputeDeltaEn(R, Mu), 0.0);
	}
);
