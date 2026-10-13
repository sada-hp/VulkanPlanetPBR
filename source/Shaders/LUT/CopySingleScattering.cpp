#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderPrecomputeLUT::CopySingleScatteringPS)
(
    layout(location = 0) in vec3 UVW;
    layout(location = 0) out vec4 Scattering;

    layout(binding = 0) uniform sampler3D DeltaSR;
    layout(binding = 1) uniform sampler3D DeltaSM;

    void main()
    {
        vec3 SampleUVW = vec3(gl_FragCoord.xy, UVW.z + 0.5) / vec3(textureSize(DeltaSR, 0));

        vec4 Ray = texture(DeltaSR, SampleUVW);
        vec4 Mie = texture(DeltaSM, SampleUVW);

        Scattering = vec4(Ray.rgb, Mie.r);
    }
);
