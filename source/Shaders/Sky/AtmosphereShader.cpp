#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaders::AtmospherePS)
(
	layout(location = 0) in vec2 UV;
	layout(location = 0) out vec4 Color;

	// binding = 0 -> UBO
	layout(binding = 1) uniform sampler2D IrradianceLUT;
	layout(binding = 2) uniform sampler3D InscatteringLUT;
	layout(binding = 3) uniform sampler2D TransmittanceLUT;

	// TODO : use scene depth

	vec3 Tonemap(vec3 L)
	{
		L = L / (0.15 * L + vec3(1.0));

		L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
		L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
		L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);

		return L;
	}

	void main()
	{
		vec4 frag_pos = vec4(UV * 2.0 - 1.0, 0.0, 1.0);
		frag_pos = UBO.view_proj_inv * frag_pos;
		frag_pos.xyz /= frag_pos.w;

		vec3 view_dir = normalize(frag_pos.xyz - UBO.eye_pos.xyz);

		vec3 S = SkyScattering(TransmittanceLUT, InscatteringLUT, UBO.eye_pos.xyz, view_dir.xyz, UBO.sun_dir.xyz);
		Color = vec4(Tonemap(S), 0.0);
	}
);