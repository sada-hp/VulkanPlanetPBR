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
		const int kernel[4] = { 1, 4, 12, 24 };

		noise_out = vec4(0.0);
		for (uint i = 0; i < 4; i++)
		{
			noise_out[i] = 1.0 - fbm_worley(UVW, Settings.frequency * kernel[i], Settings.octaves);
		}
	}
);

ShaderCodeDefinition(GShaders::CloudsCommonPS)
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
	const float BottomBound = Rg + 0.25 * Rdelta;
	const float TopBound = Rg + 0.60 * Rdelta;
	const float BoundDelta = TopBound - BottomBound;

	float GetAbsorbtion()
	{
		return mix(0.25, 0.0025, Scattering.a);
	}

		vec3 GetUV(vec3 pos, float scale)
	{
		return scale * vec3(pos / TopBound) + UBO.time * 1e-3;
	}

	float GetHeight(vec3 pos)
	{
		return saturate((length(pos) - BottomBound) / BoundDelta);
	}

	float SampleShape(vec3 pos, float heightFraction, float mip)
	{
		vec3 sample_uv = GetUV(pos, 15.0);
		float base_cloud = textureLod(CloudPerlinWorley, sample_uv, mip).r;

		float heightInverted = 1.0 - heightFraction;

		float h1 = pow(heightInverted, 1.25);
		float h2 = saturate(remap(heightFraction, 0.0, 0.125, 0.0, 1.0));

		float shape_mod = 1.0 - Coverage * h1 * h2;

		return saturate(Coverage * remap(base_cloud, shape_mod, 1.0, 0.0, 1.0));
	}

	float SampleDensity(vec3 pos, float heightFraction, float mip)
	{
		float cloud = SampleShape(pos, heightFraction, mip);

		if (cloud != 0.0 || Scattering.a > 1e-2)
		{
			vec3 sample_uv = GetUV(pos, 350.0);
			vec4 high_freq_noise = textureLod(CloudHighFrequency, sample_uv, mip);
			float fbm = 0.625 * high_freq_noise.r + 0.25 * high_freq_noise.g + 0.125 * high_freq_noise.b;

			float freq_mod = mix(fbm, 1.0 - fbm, saturate(heightFraction * 15));
			freq_mod = mix(freq_mod * 0.15, freq_mod * 0.45, heightFraction);

			cloud = remap(cloud, freq_mod, 1.0, 0.0, 1.0);
		}

		return GetAbsorbtion() * saturate(cloud);
	}

	bool SearchCloud(inout vec3 ray_pos, vec3 end)
	{
		const float ray_length = distance(ray_pos, end);
		const float steps = ceil(mix(32.f, 96.f, saturate(ray_length / (3.0 * BoundDelta))));

		float ds = ray_length / steps;
		const vec3 ray_dir = normalize(end - ray_pos);

		bool found = false;
		float sample_density = 0.0;
		for (float i = 0; i < steps; i++)
		{
			vec3 temp_pos = ray_pos + ray_dir * ds;
			sample_density = SampleDensity(temp_pos, GetHeight(temp_pos), 0);

			if (sample_density != 0)
			{
				if (found)
					return true;

				ds = ds / steps;
				found = true;
			}
			else
				ray_pos = temp_pos;
		}

		return found;
	}
);

ShaderCodeDefinition(GShaders::CloudsPS)
(
	float MultiScatter(float phi, float e)
	{
		float luminance = 0.0;
		float a = 1.0, b = 1.0, c = 1.0;
		for (int i = 0; i < 5; i++)
		{
			luminance += b * HGDPhase(c * phi, c * MieG) * BeerLambert(e * a, 1);
			a *= 0.75;
			c *= 0.25;
			b *= 0.25;
		}

		return luminance;
	}

	float SampleCone(vec3 pos, float len, float mip)
	{
		float cone_density = 0.0;
		const vec3 dir = UBO.sun_dir.xyz;

		const vec3 kernel[] =
		{
			vec3(0.0, 0.0, 0.0),
			vec3(0.57735, 0.57735, 0.57735),
			vec3(-0.677686, 0.690636, 0.252514),
			vec3(0.289651, 0.1549, -0.944515),
			vec3(0.871223, 0.400051, 0.284481),
			vec3(0.0914922, 0.494058, -0.864602),
			vec3(-0.488603, 0.531976, 0.691569)
		};

		float ds = len / 6.f;
		for (int i = 0; i <= 6; i++)
		{
			vec3 cone_dir = (dir + kernel[i] * float(i)) * ds;
			pos += cone_dir;

			cone_density += SampleDensity(pos, GetHeight(pos), mip) * length(cone_dir);
			ds *= i < 4 ? 1.0 : 2.0;
		}

		return GetAbsorbtion() * cone_density;
	}

	float StepRaySmooth(float ds, float w)
	{
		return max(ds * smoothstep(0.0, 2.0, w) * 5.3262290811553903259871638305242, 0.1 * ds);
	}

	vec3 EvaluateLight(vec3 pos, float rdotl, float base_mip)
	{
		const vec3 dir = UBO.sun_dir.xyz;
		const float len = 0.085 * BoundDelta;

		float density = SampleCone(pos, len, base_mip);
		float transmittance = MultiScatter(rdotl, density);

		return vec3(MaxLightIntensity * transmittance);
	}

	void MarchToCloud(vec3 begin, vec3 end)
	{
		const vec3 sun_dir = UBO.sun_dir.xyz;
		const vec3 eye = UBO.eye_pos.xyz;
		const float Re = length(eye);

		const float distance_to_ray = distance(eye, begin);
		const float ray_length = distance(begin, end);
		const float max_distance = sphere_intersection(vec3(0, Rg, 0), vec3(1, 0, 0), vec3(0.0), BottomBound);

		const float steps_weight = saturate(ray_length / (3.0 * BoundDelta));
		const float steps = ceil(mix(16.f, 64.f, steps_weight));

		const float mip_weight = saturate(distance_to_ray / max_distance);
		const float target_mip = max(mix(-4.f, 2.f, mip_weight), 0.0);

		const vec3 eye_n = normalize(eye);
		const vec3 ray_dir = normalize(end - begin);

		const float edotl = saturate(dot(sun_dir, eye) / Re);
		const float rdotl = saturate(dot(sun_dir, ray_dir));

		vec3 ray_pos = begin;
		float height = GetHeight(ray_pos);
		float ray_depth = height * BoundDelta + BottomBound;

		Scattering = vec4(0.0, 0.0, 0.0, 1.0);
		float ds = ray_length / steps;

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
			float sample_density = SampleDensity(ray_pos, height, target_mip);
			if (sample_density > 0.0)
			{
				float extinction = sample_density;
				float transmittance = BeerLambert(extinction, step_size);

				float pdotl = dot(sun_dir, ray_pos) / ray_depth;
				vec3 T = GetTransmittanceWithShadow(TransmittanceLUT, ray_depth, pdotl, Re, edotl);
				vec3 S = GetAEP(TransmittanceLUT, InscatteringLUT, old_pos, old_depth, ray_pos, ray_depth, ray_dir, sun_dir);

				vec3 E = extinction * EvaluateLight(ray_pos, rdotl, target_mip);
				E = (E - E * transmittance) / extinction;

				Scattering.rgb += Scattering.a * (T * E + S);
				Scattering.a *= transmittance;
			}

			if (Scattering.a < 1e-5)
				break;
		}

		Scattering.rgb += GetAEP(TransmittanceLUT, InscatteringLUT, eye, begin, sun_dir) * (1.0 - Scattering.a);
		Scattering.a = 1.0 - Scattering.a;
	}

	void main()
	{
		vec4 screen_pos = vec4(2.0 * UV - 1.0, 0.0, 1.0);
		screen_pos = UBO.view_proj_inv * screen_pos;

		Scattering = vec4(0.0);
		vec3 eye_pos = UBO.eye_pos.xyz;
		vec3 ray_dir = normalize(screen_pos.xyz / screen_pos.w);

		vec3 ray_origin, ray_end;
		if (get_ray(eye_pos, ray_dir, ray_origin, ray_end))
		{
			MarchToCloud(ray_origin, ray_end);
		}
	}
);

ShaderCodeDefinition(GShaders::CloudsSetupUnderPS)
(
	bool get_ray(vec3 eye, vec3 dir, out vec3 ray_origin, out vec3 ray_end)
	{
		float dg = sphere_intersection(eye, dir, vec3(0.0), Rg);
		float dt = sphere_intersection(eye, dir, vec3(0.0), TopBound);
		float db = sphere_intersection(eye, dir, vec3(0.0), BottomBound);
		
		ray_origin = eye + dir * db;
		ray_end = eye + dir * dt;
		
		return dg == Invalid && SearchCloud(ray_origin, ray_end);
	}
);

ShaderCodeDefinition(GShaders::CloudsSetupBetweenPS)
(
	bool get_ray(vec3 eye, vec3 dir, out vec3 ray_origin, out vec3 ray_end)
	{
		float db = sphere_intersection(eye, dir, vec3(0.0), BottomBound);
		float dt = sphere_intersection(eye, dir, vec3(0.0), TopBound);
		float dg = sphere_intersection(eye, dir, vec3(0.0), Rg);

		bool groundVisible = dg != Invalid;
		ray_end = eye + dir * mix(dt, db, float(groundVisible));
		ray_origin = eye;

		return SearchCloud(ray_origin, ray_end);
	}
);

ShaderCodeDefinition(GShaders::CloudsSetupAbovePS)
(
	bool get_ray(vec3 eye, vec3 dir, out vec3 ray_origin, out vec3 ray_end)
	{
		float db = sphere_intersection(eye, dir, vec3(0.0), BottomBound);
		vec2 dt = sphere_intersections(eye, dir, vec3(0.0), TopBound);
		float dg = sphere_intersection(eye, dir, vec3(0.0), Rg);

		float dt_min = min(dt.x, dt.y);
		float dt_max = max(dt.x, dt.y);
		float dt_bot = min(db, dg);

		ray_end = eye + dir * min(dt_bot, dt_max);
		ray_origin = eye + dir * dt_min;

		return (dt.x != Invalid || dt.y != Invalid) && SearchCloud(ray_origin, ray_end);
	}
);