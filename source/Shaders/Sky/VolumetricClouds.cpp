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

		perlinWorley = remap(perlin_part, 0.0, 1.0, worley_part, 1.0);
		// perlinWorley = remap(worley_part, 0.0, 1.0, 0.0, perlin_part);

		float fbm0 = fbm_worley(UVW, Settings.frequency_worley, 3);
		float fbm1 = fbm_worley(UVW, Settings.frequency_worley * 2, 3);
		float fbm2 = fbm_worley(UVW, Settings.frequency_worley * 4, 3);

		float fbm = fbm0 * 0.625 + fbm1 * 0.25 + fbm2 * 0.125;
		perlinWorley = remap(perlinWorley, fbm, 1.0, 0.15, 1.0);
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
		const int kernel[4] = { 1, 8, 16, 24 };

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

	const float Coverage = 0.5;
	const float Absorbtion = 8e-2;
	const float BottomBound = Rg + 0.25 * (Rt - Rg);
	const float TopBound    = Rg + 0.60 * (Rt - Rg);
	const float BoundDelta  = TopBound - BottomBound;

	vec3 GetUV(vec3 pos, float scale)
	{
		return scale * vec3(pos / TopBound) + UBO.time * 1e-3;
	}

	float GetHeight(vec3 pos)
	{
		return saturate((length(pos) - BottomBound) / BoundDelta);
	}

	float SampleShape(vec3 pos, float heightFraction)
	{
		vec3 sample_uv = GetUV(pos, 15.0);
		float base_cloud = texture(CloudPerlinWorley, sample_uv).r;

		float heightInverted = 1.0 - heightFraction;

		float h1 = pow(heightInverted, 1.5);
		float h2 = saturate(remap(heightFraction, 0.0, 0.2, 0.0, 1.0));

		float shape_mod = 1.0 - Coverage * h1 * h2;

		return saturate(Coverage * remap(base_cloud, shape_mod, 1.0, 0.0, 1.0));
	}

	float SampleDensity(vec3 pos, float heightFraction)
	{
		float cloud = SampleShape(pos, heightFraction);

		if (cloud != 0.0)
		{
			vec3 sample_uv = GetUV(pos, 250.0);
			vec4 high_freq_noise = texture(CloudHighFrequency, sample_uv);
			float fbm = 0.625 * high_freq_noise.r + 0.25 * high_freq_noise.g + 0.125 * high_freq_noise.b;

			float freq_mod = mix(fbm, 1.0 - fbm, saturate(heightFraction * 25));
			freq_mod = mix(freq_mod * 0.15, freq_mod * 0.45, smoothstep(0.0, 1.0, heightFraction));

			cloud = remap(cloud, freq_mod, 1.0, 0.0, 1.0);
		}

		return Absorbtion * saturate(cloud);
	}

	float MultiScatter(float phi, float e, float ds)
	{
		float luminance = 0.0;
		float a = 1.0, b = 1.0, c = MieG;
		for (int i = 0; i < 15; i++)
		{
			luminance += b * HGDPhase(phi, c) * Powder(e * a, ds);
			a *= 0.5;
			b *= 0.45;
			c *= 0.75;
		}

		return luminance;
	}

	float SampleCone(vec3 pos, float len)
	{
		float cone_density = 0.0;
		const vec3 dir = UBO.sun_dir.xyz;

		const vec3 kernel[] =
		{
			vec3(0.57735, 0.57735, 0.57735),
			vec3(-0.677686, 0.690636, 0.252514),
			vec3(0.289651, 0.1549, 0.944515),
			vec3(0.871223, 0.400051, 0.284481),
			vec3(0.0914922, 0.494058, 0.864602),
			vec3(0.488603, 0.531976, 0.691569)
		};

		float ds = len / 6.f;
		for (int i = 0; i < 6; i++)
		{
			pos += (dir + kernel[i] * float(i)) * ds;
			cone_density += SampleDensity(pos, GetHeight(pos)) * ds;
			ds *= i < 4 ? 1.0 : 2.0;
		}

		return Absorbtion * cone_density;
	}

	float StepRaySmooth(float ds, float w)
	{
		return smoothstep(0.0, 2.0, w) * ds * 5.3262290811553903259871638305242;
	}

	vec3 EvaluateLight(vec3 pos, float R)
	{
		const vec3 dir = UBO.sun_dir.xyz;
		const float len = min(sphere_intersection(pos, dir, vec3(0.0), TopBound), 0.25 * BoundDelta);

		float density = SampleCone(pos, len);
		float transmittance = BeerLambert(density, 1.0);

		return vec3(MaxLightIntensity * transmittance);
	}

	bool SearchCloud(inout vec3 ray_pos, vec3 end)
	{
		const float steps = 64;
		float ds = distance(end, ray_pos) / steps;
		const vec3 ray_dir = normalize(end - ray_pos);

		bool found = false;
		float sample_density = 0.0;
		for (float i = 0; i < steps; i++)
		{
			ray_pos += ray_dir * ds;
			sample_density = SampleDensity(ray_pos, GetHeight(ray_pos));

			// refine
			if (sample_density != 0)
			{
				ray_pos -= ray_dir * ds;
				ds = ds / 2.0;
				found = true;
			}
		}

		return found;
	}

	void MarchToCloud(vec3 begin, vec3 end)
	{
		const vec3 sun_dir = UBO.sun_dir.xyz;
		const vec3 eye = UBO.eye_pos.xyz;
		const float Re = length(eye);

		const vec3 eye_n = normalize(eye);
		const vec3 ray_dir = normalize(end - begin);
		const float horizon = max(dot(ray_dir, eye_n), 0.0);
		const float steps = ceil(mix(128.f, 48.f, horizon));

		const float edotl = dot(sun_dir, eye) / Re;
		const float rdotl = dot(sun_dir, ray_dir);

		vec3 ray_pos = begin;
		float height = GetHeight(ray_pos);
		float ray_depth = height * BoundDelta + BottomBound;

		float ds = distance(end, begin) / steps;
		for (float i = 0; i < steps; i++)
		{
			// save state
			vec3 old_pos = ray_pos;
			float old_height = height;
			float old_depth = ray_depth;
			// update step size
			float step_w = i / steps;
			float step_size = StepRaySmooth(ds, step_w);
			// increment ray
			ray_pos += ray_dir * step_size;
			// update ray
			height = GetHeight(ray_pos);
			ray_depth = height * BoundDelta + BottomBound;
			// sample depth
			float sample_density = SampleDensity(ray_pos, height);
			if (sample_density > 0.0)
			{
				float extinction = sample_density;
				float phase = MultiScatter(rdotl, extinction, step_size);
				float transmittance = BeerLambert(extinction, step_size);

				vec3 T = GetTransmittanceWithShadow(TransmittanceLUT, ray_depth, dot(sun_dir, ray_pos) / ray_depth, Re, edotl);
				vec3 S = GetAEP(TransmittanceLUT, InscatteringLUT, old_pos, old_depth, ray_pos, ray_depth, ray_dir, sun_dir);

				vec3 E = extinction * phase * EvaluateLight(ray_pos, ray_depth);
				E = (E - E * transmittance) / extinction;

				Scattering.rgb += Scattering.a * (T * E + S);
				Scattering.a *= transmittance;
			}

			if (Scattering.a < 1e-5)
				break;
		}

		Scattering.rgb += smootherstep(0.0, 1.0, Coverage) * GetAEP(TransmittanceLUT, InscatteringLUT, eye, begin, sun_dir) * (1.0 - Scattering.a);
	}

	void main()
	{
		vec4 screen_pos = vec4(2.0 * UV - 1.0, 0.0, 1.0);
		screen_pos = UBO.view_proj_inv * screen_pos;

		vec3 eye_pos = UBO.eye_pos.xyz;
		vec3 ray_dir = normalize(screen_pos.xyz / screen_pos.w);

		Scattering = vec4(0.0, 0.0, 0.0, 1.0);
		if (sphere_intersection(eye_pos, ray_dir, vec3(0.0), Rg) == 0.0)
		{
			float dt = sphere_intersection(eye_pos, ray_dir, vec3(0.0), TopBound);
			float db = sphere_intersection(eye_pos, ray_dir, vec3(0.0), BottomBound);

			vec3 ray_origin = eye_pos + ray_dir * min(dt, db);
			vec3 ray_end = eye_pos + ray_dir * max(dt, db);

			if (SearchCloud(ray_origin, ray_end))
				MarchToCloud(ray_origin, ray_end);
		}
	}
);