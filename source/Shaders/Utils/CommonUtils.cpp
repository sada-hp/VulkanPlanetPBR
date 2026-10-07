#include "Shaders/ShaderLibrary.hpp"

ShaderCodeDefinition(GShaderUtils::UtilsCommon)
(
    float smootherstep(float e0, float e1, float x)
    {
        x = clamp((x - e0) / (e1 - e0), 0.0, 1.0);
        return x * x * x * (x * (6.0 * x - 15.0) + 10.0);
    }

    float inverse_smoothstep(float e0, float e1, float x)
    {
        x = clamp((x - e0) / (e1 - e0), 0.0, 1.0);
        return 0.5 - sin(asin(1.0 - 2.0 * x) / 3.0);
    }

    float ridge_smoothstep(float e0, float e1, float x)
    {
        x = clamp((x - e0) / (e1 - e0), 0.0, 1.0);
        float sm = x * x * (3 - 2 * x);
        float inv = 0.5 - sin(asin(1.0 - 2.0 * x) / 3.0);

        return sm * inv;
    }

    float saturate(float x)
    {
        return clamp(x, 0.0, 1.0);
    }

    vec2 saturate(vec2 x)
    {
        return clamp(x, 0.0, 1.0);
    }

    vec3 saturate(vec3 x)
    {
        return clamp(x, 0.0, 1.0);
    }

    vec4 saturate(vec4 x)
    {
        return clamp(x, 0.0, 1.0);
    }

    float dampen(float Value, float Factor)
    {
        return saturate(Value - Factor) / (1.0 - Factor);
    }

    float saturateAngle(float x)
    {
        return clamp(x, -1.0, 1.0);
    }

    vec2 saturateAngle(vec2 x)
    {
        return clamp(x, -1.0, 1.0);
    }

    vec3 divide_w(vec4 v)
    {
        return v.xyz / v.w;
    }

    vec3 sphere_intersection(vec3 ro, vec3 rd, vec3 so, float radius)
    {
        float radius2 = radius * radius;

        vec3 L = ro - so;
        float a = dot(rd, rd);
        float b = 2.0 * dot(rd, L);
        float c = dot(L, L) - radius2;
        float discr = b * b - 4.0 * a * c;

        vec2 t = vec2(0.0);
        t.x = mix(0xffffffff, -b - sqrt(discr) / 2, float(discr >= 0.0));
        t.y = mix(0xffffffff, -b + sqrt(discr) / 2, float(discr >= 0.0));

        return ro + rd * min(t.x, t.y);
    }
);