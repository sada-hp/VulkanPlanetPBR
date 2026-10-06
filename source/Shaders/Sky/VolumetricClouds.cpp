#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderNoise::PerlinWorleyPS)
(
\n #ifndef _3D_NOISE \n
	layout(location = 0) in vec2 UVW;
\n #else \n
	layout(location = 0) in vec3 UVW;
\n #endif \n
	layout(location = 0) out float perlin_worley;

	layout(push_constant) uniform constants
	{
		uint frequency_worley;
		uint frequency_perlin;
		uint seed;
	}
	Settings;

	void main()
	{
		NOISE_SEED = Settings.seed;

		float worley01 = 1.0 - worley(UVW, Settings.frequency_worley * 4);
		float worley02 = 1.0 - worley(UVW, Settings.frequency_worley * 8);
		float worley03 = 1.0 - worley(UVW, Settings.frequency_worley * 14);

		float perlin = fbm_perlin(UVW, Settings.frequency_perlin, 8) * 0.5 + 0.5;
		float fbm_worley = (worley01 * 1.0 + worley02 * 0.5 + worley03 * 0.25) / 1.75;

		perlin_worley = remap(perlin, 0.0, 1.0, fbm_worley, 1.0);
	}
);

ShaderCodeDefinition(GShaderNoise::WorleyPS)
(
\n #ifndef _3D_NOISE \n
	layout(location = 0) in vec2 UVW;
\n #else \n
	layout(location = 0) in vec3 UVW;
\n #endif \n

	layout(location = 0) out vec4 worley;

	layout(push_constant) uniform constants
	{
		uint frequency;
		uint octaves;
		uint seed;
	}
	Settings;

	void main()
	{
		worley = vec4(0.0);

		int kernel[4] = { 3, 8, 12, 13 };
		for (uint i = 0; i < 4; i++)
		{
			worley[i] = 1.0 - fbm_worley(cell_loc, Settings.frequency * kernel[i], Settings.octaves);
		}
	}
);

ShaderCodeDefinition(GShaders::CloudsPS)
(
	layout(location = 0) in vec2 UV;
	layout(location = 0) out vec4 Color;

	// binding = 0 -> UBO
	layout(binding = 1) uniform sampler2D IrradianceLUT;
	layout(binding = 2) uniform sampler3D InscatteringLUT;
	layout(binding = 3) uniform sampler2D TransmittanceLUT;
	layout(binding = 4) uniform sampler3D CloudLowFrequency;
	layout(binding = 5) uniform sampler3D CloudHighFrequency;
	layout(binding = 6) uniform sampler3D CloudPerlinWorleyFrequency;

	void main()
	{

	}
);