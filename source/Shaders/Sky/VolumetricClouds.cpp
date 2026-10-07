#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderNoise::PerlinWorleyPS)
(
\n #ifndef _3D_NOISE \n
	layout(location = 0) in vec2 UVW;
\n #else \n
	layout(location = 0) in vec3 UVW;
\n #endif \n
	layout(location = 0) out float noise_out;

	layout(push_constant) uniform constants
	{
		uint layers;
		uint frequency_worley;
		uint frequency_perlin;
		uint seed;
	}
	Settings;

	void main()
	{
		NOISE_SEED = Settings.seed;

		float worley01 = 1.0 - worley(UVW, Settings.frequency_worley);
		float worley02 = 1.0 - worley(UVW, Settings.frequency_worley * 3);
		float worley03 = 1.0 - worley(UVW, Settings.frequency_worley * 5);

		float perlin = fbm_perlin(UVW, Settings.frequency_perlin, 8) * 0.5 + 0.5;
		float fbm_worley = (worley01 * 1.0 + worley02 * 0.5 + worley03 * 0.25) / 1.75;

		noise_out = remap(perlin, 0.0, 1.0, fbm_worley, 1.0);
	}
);

ShaderCodeDefinition(GShaderNoise::WorleyPS)
(
\n #ifndef _3D_NOISE \n
	layout(location = 0) in vec2 UVW;
\n #else \n
	layout(location = 0) in vec3 UVW;
\n #endif \n

	layout(location = 0) out vec4 noise_out;

	layout(push_constant) uniform constants
	{
		uint layers;
		uint frequency;
		uint octaves;
		uint seed;
	}
	Settings;

	void main()
	{
		NOISE_SEED = Settings.seed;

		const int kernel[4] = { 1, 2, 4, 6 };

		noise_out = vec4(0.0);
		for (uint i = 0; i < 4; i++)
			noise_out[i] = 1.0 - fbm_worley(UVW, Settings.frequency * kernel[i], Settings.octaves);
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
	layout(binding = 6) uniform sampler3D CloudPerlinWorley;

	const float TopBound = 3.0 * (Rt - Rg) / 4.0;
	const float BottomBound = (Rt - Rg) / 4.0;

	vec3 GetUV(vec3 pos, float h)
	{
		return vec3(pos.x, h, pos.z);
	}

	float SampleDensity(vec3 pos, float h)
	{
		vec3 sample_uv = GetUV(pos, h);

		vec4 low_frequency_noise = texture(CloudLowFrequency, sample_uv);
		float low_frequency_fbm = low_frequency_noise.g * 0.625 + low_frequency_noise.b * 0.25 + low_frequency_noise.a * 0.125;

		float density = texture(CloudPerlinWorley, sample_uv).r;
		density = remap(density, (1.0 - low_frequency_fbm), 1.0, 0.0, 1.0);

		return density;
	}

	vec4 MarchToCloud(vec3 begin, vec3 end)
	{
		const float steps = 5;
		const float density = 1.0;
		const vec3 dh = (end - begin) / steps;

		vec3 ray_pos = begin;
		vec4 scattering = vec4(1.0);
		for (float i = 0; i < steps; i++)
		{
			float sample_density = density * SampleDensity(ray_pos, i / steps);
			scattering *= sample_density;
			ray_pos += dh;
		}

		return scattering;
	}

	void main()
	{
		vec4 screen_pos = vec4(2.0 * UV - 1.0, 0.0, 1.0);
		screen_pos = UBO.view_proj_inv * screen_pos;
		screen_pos.xyz /= screen_pos.w;

		vec3 eye_pos = UBO.eye_pos.xyz - vec3(0.0, Rg, 0.0);
		vec3 ray_dir = normalize(screen_pos.xyz - UBO.eye_pos.xyz);
		vec3 ray_origin = sphere_intersection(eye_pos, ray_dir, vec3(0.0), BottomBound);
		vec3 ray_end = sphere_intersection(eye_pos, ray_dir, vec3(0.0), TopBound);

		Color = MarchToCloud(ray_origin, ray_end);
	}
);