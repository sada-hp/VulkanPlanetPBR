#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaders::TonemapPS)
(
	layout(location = 0) out vec4 Color;
	layout(location = 0) in vec2 UV;

	layout(set = 0, binding = 0) uniform sampler2D ColorHR;

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
		Color = vec4(Tonemap(texture(ColorHR, UV).rgb), 1.0);
	}
);