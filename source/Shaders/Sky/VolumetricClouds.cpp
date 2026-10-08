#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderNoise::PerlinWorleyPS)
(
\n #ifndef _3D_NOISE \n
	layout(location = 0) in vec2 UVW;
\n #else \n
	layout(location = 0) in vec3 UVW;
\n #endif \n
	layout(location = 0) out float perlinWorley;

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
		float worley01 = 1.0 - worley(UVW, Settings.frequency_worley * 2);
		float worley02 = 1.0 - worley(UVW, Settings.frequency_worley * 8);
		float worley03 = 1.0 - worley(UVW, Settings.frequency_worley * 16);

		float perlin_part = fbm_perlin(UVW, Settings.frequency_perlin, 3);
		float worley_part = worley01 * 0.625 + worley02 * 0.25 + worley03 * 0.125;

		// perlinWorley = remap(perlin_part, worley_part, 1.0, 0.0, 1.0);
		perlinWorley = remap(worley_part, 0.0, 1.0, 0.0, perlin_part);

		float fbm0 = fbm_worley(UVW, Settings.frequency_worley, 3);
		float fbm1 = fbm_worley(UVW, Settings.frequency_worley * 2, 3);
		float fbm2 = fbm_worley(UVW, Settings.frequency_worley * 4, 3);

		float fbm = fbm0 * 0.625 + fbm1 * 0.25 + fbm2 * 0.125;
		perlinWorley = remap(perlinWorley, fbm, 1.0, 0.25, 1.0);
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
		const int kernel[4] = { 1, 2, 4, 6 };

		noise_out = vec4(0.0);
		for (uint i = 0; i < 4; i++)
		{
			noise_out[i] = 1.0 - fbm_worley(UVW, Settings.frequency * kernel[i], Settings.octaves);
		}
	}
);

ShaderCodeDefinition(GShaders::CloudsPS)
(
	layout(location = 0) in vec2 UV;
	layout(location = 0) out vec4 Scattering;

	// binding = 0 -> UBO
	layout(binding = 1) uniform sampler2D IrradianceLUT;
	layout(binding = 2) uniform sampler3D InscatteringLUT;
	layout(binding = 3) uniform sampler2D TransmittanceLUT;
	layout(binding = 4) uniform sampler3D CloudPerlinWorley;
	layout(binding = 5) uniform sampler3D CloudHighFrequency;

	const float BottomBound = Rg + 0.25 * (Rt - Rg);
	const float TopBound    = Rg + 0.6 * (Rt - Rg);
	const float BoundDelta  = TopBound - BottomBound;

	vec3 GetUV(vec3 pos, float scale)
	{
		return scale * vec3(pos / TopBound);
	}

	float SampleShape(vec3 pos)
	{
		vec3 sample_uv = GetUV(pos, 15.0);
		float base_cloud = texture(CloudPerlinWorley, sample_uv).r;

		float heightFraction = (length(pos) - BottomBound) / BoundDelta;
		float grad = remap(heightFraction, 0.1, 0.3, 0.0, 1.0) * remap(heightFraction, 0.4, 0.6, 1.0, 0.0);
		base_cloud *= saturate(grad);

		return saturate(0.25 * base_cloud);
	}

	float SampleDensity(vec3 pos)
	{
		float shape = SampleShape(pos);
		return shape;
	}

	float MultiScatter(float phi, float e, float ds)
	{
		float luminance = 0.0;
		float a = 1.0, b = 1.0, c = 1.0;
		for (int i = 0; i < 5; i++)
		{
			luminance += b * HGDPhaseCloud(phi, c) * BeerLambert(e * a, ds);
			a *= 0.5;
			b *= 0.45;
			c *= 0.75;
		}

		return luminance;
	}

	float EvaluateLight(vec3 pos)
	{
		const vec3 dir = UBO.sun_dir.xyz;
		const float len = sphere_intersection(pos, dir, vec3(0.0), TopBound);

		const int steps = 6;
		const float ds = len / float(steps);

		float density = 1.0;
		for (int i = 0; i < steps; i++)
		{
			pos += dir * ds;

			float density_sample = SampleDensity(pos);

			if (density_sample > 0.0)
				density = density * density_sample;
		}

		return MaxLightIntensity * density;
	}

	void MarchToCloud(vec3 eye, vec3 begin, vec3 end)
	{
		const float steps = 128;
		const vec3 ray_dir = normalize(end - begin);
		const float phi = dot(UBO.sun_dir.xyz, ray_dir);

		vec3 ray_pos = begin;
		float ds = distance(end, begin) / steps;

		float sample_density = 0.0;
		for (float i = 0; i < steps; i++)
		{
			sample_density = SampleDensity(ray_pos);

			if (sample_density != 0)
			{
				ray_pos -= ray_dir * ds;
				break;
			}

			ray_pos += ray_dir * ds;
		}

		if (sample_density == 0.0)
			return;

		ds = distance(end, ray_pos) / steps;
		for (float i = 0; i < steps; i++)
		{
			sample_density = SampleDensity(ray_pos);

			if (sample_density > 0.0)
			{
				float extinction = sample_density + 1e-6;
				float transmittance = BeerLambert(extinction, ds);

				SAtmosphere Atmosphere;
				AerialPerspective(TransmittanceLUT, IrradianceLUT, InscatteringLUT, eye, ray_pos, UBO.sun_dir.xyz, Atmosphere);

				vec3 E = extinction * (Atmosphere.L * vec3(EvaluateLight(ray_pos) * MultiScatter(phi, extinction, ds)));
				E = (E - E * transmittance) / extinction;

				Scattering.rgb += Scattering.a * (E + Atmosphere.S);
				Scattering.a *= transmittance;
			}

			ray_pos += ray_dir * ds;
		}
	}

	void main()
	{
		vec4 screen_pos = vec4(2.0 * UV - 1.0, 0.0, 1.0);
		screen_pos = UBO.view_proj_inv * screen_pos;
		Scattering = vec4(0.0, 0.0, 0.0, 1.0);

		vec3 eye_pos = UBO.eye_pos.xyz;
		vec3 ray_dir = normalize(screen_pos.xyz / screen_pos.w);

		float dg = sphere_intersection(eye_pos, ray_dir, vec3(0.0), Rg);

		if (dg == 0.0)
		{
			float dt = sphere_intersection(eye_pos, ray_dir, vec3(0.0), TopBound);
			float db = sphere_intersection(eye_pos, ray_dir, vec3(0.0), BottomBound);

			vec3 ray_origin = eye_pos + ray_dir * min(dt, db);
			vec3 ray_end = eye_pos + ray_dir * max(dt, db);
			MarchToCloud(eye_pos, ray_origin, ray_end);
		}
	}
);