#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaders::TonemapPS)
(
	layout(location = 0) out vec4 Color;
	layout(location = 0) in vec2 UV;

	layout(set = 0, binding = 0) uniform sampler2D ColorHR;

	vec3 Tonemap(vec3 L)
	{
		L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
		L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
		L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);

		return L;
	}

    vec3 FXAA()
    {
        float FXAA_SPAN_MAX = 8.0;
        float FXAA_REDUCE_MUL = 0.125f;
        float FXAA_REDUCE_MIN = 1.0 / 256.0;

        vec3 rgbNW = Tonemap(texelFetch(ColorHR, ivec2(gl_FragCoord.xy + vec2(-1.0, -1.0)), 0).xyz);
        vec3 rgbNE = Tonemap(texelFetch(ColorHR, ivec2(gl_FragCoord.xy + vec2(1.0, -1.0)), 0).xyz);
        vec3 rgbSW = Tonemap(texelFetch(ColorHR, ivec2(gl_FragCoord.xy + vec2(-1.0, 1.0)), 0).xyz);
        vec3 rgbSE = Tonemap(texelFetch(ColorHR, ivec2(gl_FragCoord.xy + vec2(1.0, 1.0)), 0).xyz);
        vec3 rgbM = Tonemap(texelFetch(ColorHR, ivec2(gl_FragCoord.xy), 0).xyz);

        vec3 luma = vec3(0.299, 0.587, 0.114);
        float lumaNW = dot(rgbNW, luma);
        float lumaNE = dot(rgbNE, luma);
        float lumaSW = dot(rgbSW, luma);
        float lumaSE = dot(rgbSE, luma);
        float lumaM = dot(rgbM, luma);

        float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
        float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

        vec2 dir;
        dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
        dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

        float dirReduce = max
        (
            (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
            FXAA_REDUCE_MIN
        );

        float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

        dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
            max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
                dir * rcpDirMin)) / textureSize(ColorHR, 0).xy;

        vec3 rgbA = (1.0 / 2.0) * (
            texture(ColorHR, UV.xy + dir * (1.0 / 3.0 - 0.5)).xyz +
            texture(ColorHR, UV.xy + dir * (2.0 / 3.0 - 0.5)).xyz);
        vec3 rgbB = rgbA * (1.0 / 2.0) + (1.0 / 4.0) * (
            texture(ColorHR, UV.xy + dir * (0.0 / 3.0 - 0.5)).xyz +
            texture(ColorHR, UV.xy + dir * (3.0 / 3.0 - 0.5)).xyz);
        float lumaB = dot(Tonemap(rgbB), luma);

        if ((lumaB < lumaMin) || (lumaB > lumaMax))
        {
            return rgbA;
        }
        else
        {
            return rgbB;
        }
    }

    vec3 toSRGB(vec3 linear)
    {
        bvec3 cutoff = lessThan(linear, vec3(0.0031308));
        vec3 higher = vec3(1.055) * pow(linear, vec3(1.0 / 2.4)) - vec3(0.055);
        vec3 lower = linear * vec3(12.92);
        return mix(higher, lower, cutoff);
    }

	void main()
	{
		vec3 HDR = FXAA();
		vec3 LDR = Tonemap(HDR);
        vec3 SRGB = toSRGB(LDR);
        float luma = dot(LDR, vec3(0.2126, 0.7152, 0.0722));

		Color = vec4(mix(LDR, SRGB, luma), 1.0);
	}
);